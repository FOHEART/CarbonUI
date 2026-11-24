/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIProcessStats.cpp
 * @brief CarbonUIProcessStats 的实现：平台采集、区间占用率与文案格式化。
 * @author CarbonUI
 * @date  2026-11-24
 * @ingroup carbonui
 *
 * @details
 * Windows 侧 CPU 时间取 GetProcessTimes 的 kernel + user（FILETIME 是 100 ns 单位），
 * 进程内存取 PROCESS_MEMORY_COUNTERS::WorkingSetSize，整机可用内存取
 * GlobalMemoryStatusEx::ullAvailPhys；Linux 侧解析 /proc/self/stat，
 * 用最后一个 ')' 跳过可能含括号与空格的 comm 字段（state 是单字符，不能按 %ld 读），
 * 字段 14/15/24 分别是 utime / stime / rss，整机可用内存解析 /proc/meminfo 的
 * MemAvailable；macOS 侧用 getrusage、mach_task_basic_info::resident_size 与
 * host_statistics64（free + inactive 页数 × 页大小）。墙钟统一用 steady_clock 的毫秒读数。
 */

#include "CarbonUIProcessStats.h"

#include <chrono>
#include <cmath>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QStringList>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#if defined(_MSC_VER)
#pragma comment(lib, "psapi.lib")
#endif
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <sys/resource.h>
#endif

namespace {

/// @brief 一 MB 的字节数；按 1024² 而不是 1000²，与任务管理器一致。
const double kBytesPerMegabyte = 1024.0 * 1024.0;

/// @brief 把非负有限值格式化成 1 位小数，并消掉 "-0.0" 这类负零。
QString fixedOneDecimal(double value)
{
  const double rounded = std::round(value * 10.0) / 10.0;
  const double normalized = (rounded == 0.0) ? 0.0 : rounded;
  return QString::number(normalized, 'f', 1);
}

/// @brief steady_clock 的毫秒读数，不受系统时间调整影响。
quint64 monotonicWallTimeMs()
{
  const std::chrono::steady_clock::duration sinceEpoch =
    std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<quint64>(
    std::chrono::duration_cast<std::chrono::milliseconds>(sinceEpoch).count());
}

#if defined(Q_OS_WIN)
/// @brief FILETIME 的两个 32 位字段合成 64 位整数。
quint64 fileTimeToUInt64(const FILETIME& fileTime)
{
  return (static_cast<quint64>(fileTime.dwHighDateTime) << 32) |
         static_cast<quint64>(fileTime.dwLowDateTime);
}

/// @brief 整机可用物理内存（Windows：GlobalMemoryStatusEx 的 ullAvailPhys）。
bool systemAvailableMemoryBytes(quint64* bytes)
{
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (!::GlobalMemoryStatusEx(&status))
  {
    return false;
  }
  *bytes = static_cast<quint64>(status.ullAvailPhys);
  return true;
}
#endif

#if defined(Q_OS_LINUX)
/// @brief 读取 sysconf 的正整数配置，失败时退回 fallback。
quint64 positiveSysConf(int name, quint64 fallback)
{
  const long value = ::sysconf(name);
  return (value > 0) ? static_cast<quint64>(value) : fallback;
}

/// @brief 整机可用物理内存（Linux：/proc/meminfo 的 MemAvailable，kB → 字节）。
/// @details MemAvailable 由内核按「MemFree + 可回收缓存」估算，比裸 MemFree 更
///          接近真实可分配量；内核 < 3.14 没有该行，返回失败显示占位符。
bool systemAvailableMemoryBytes(quint64* bytes)
{
  QFile memInfoFile(QStringLiteral("/proc/meminfo"));
  if (!memInfoFile.open(QIODevice::ReadOnly))
  {
    return false;
  }
  while (!memInfoFile.atEnd())
  {
    const QByteArray line = memInfoFile.readLine();
    if (!line.startsWith("MemAvailable:"))
    {
      continue;
    }
    // 行格式 "MemAvailable:   12345678 kB"；取第一段数值后乘 1024 换算成字节。
    const QByteArray valueToken = line.mid(sizeof("MemAvailable:") - 1).simplified();
    const int spaceIndex = valueToken.indexOf(' ');
    const QByteArray number = (spaceIndex > 0)
      ? valueToken.left(spaceIndex) : valueToken;
    bool ok = false;
    const quint64 kiloBytes = number.toULongLong(&ok);
    if (!ok)
    {
      return false;
    }
    *bytes = kiloBytes * 1024ULL;
    return true;
  }
  return false;
}
#endif

#if defined(Q_OS_MACOS)
/// @brief 整机可用物理内存（macOS：host_statistics64 的 free + inactive 页数 × 页大小）。
bool systemAvailableMemoryBytes(quint64* bytes)
{
  vm_statistics64_data_t vmInfo;
  mach_msg_type_number_t infoCount = HOST_VM_INFO64_COUNT;
  if (::host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info_t>(&vmInfo),
                          &infoCount) != KERN_SUCCESS)
  {
    return false;
  }
  vm_size_t pageSize = 0;
  if (::host_page_size(mach_host_self(), &pageSize) != KERN_SUCCESS || pageSize == 0)
  {
    return false;
  }
  // free 是完全空闲页，inactive 是「内容已换出、可立即回收」的页，两者都可分配。
  *bytes = (static_cast<quint64>(vmInfo.free_count) +
            static_cast<quint64>(vmInfo.inactive_count)) *
           static_cast<quint64>(pageSize);
  return true;
}
#endif

#if !defined(Q_OS_WIN) && !defined(Q_OS_LINUX) && !defined(Q_OS_MACOS)
/// @brief 未支持平台：无系统可用内存来源，恒返回失败（状态栏显示占位符）。
bool systemAvailableMemoryBytes(quint64* bytes)
{
  Q_UNUSED(bytes);
  return false;
}
#endif

}  // namespace

namespace CarbonUIProcessStats {

Snapshot readSnapshot()
{
  Snapshot snapshot;
  snapshot.wallTimeMs = monotonicWallTimeMs();

  // 整机可用内存与进程指标相互独立：进程侧采集失败也不影响本字段，
  // 因此在进入进程采集的早退分支之前先填好。
  snapshot.availableValid =
    systemAvailableMemoryBytes(&snapshot.availableMemoryBytes);

#if defined(Q_OS_WIN)
  FILETIME creationTime;
  FILETIME exitTime;
  FILETIME kernelTime;
  FILETIME userTime;
  if (!::GetProcessTimes(::GetCurrentProcess(), &creationTime, &exitTime,
                         &kernelTime, &userTime))
  {
    return snapshot;
  }

  PROCESS_MEMORY_COUNTERS counters;
  if (!::GetProcessMemoryInfo(::GetCurrentProcess(), &counters, sizeof(counters)))
  {
    return snapshot;
  }

  const quint64 cpu100ns = fileTimeToUInt64(kernelTime) + fileTimeToUInt64(userTime);
  snapshot.cpuTimeMs = cpu100ns / 10000ULL;
  snapshot.memoryBytes = static_cast<quint64>(counters.WorkingSetSize);
  snapshot.valid = snapshot.memoryBytes > 0;
#elif defined(Q_OS_LINUX)
  QFile statFile(QStringLiteral("/proc/self/stat"));
  if (!statFile.open(QIODevice::ReadOnly))
  {
    return snapshot;
  }

  const QByteArray content = statFile.readAll();
  // comm 字段被括号包住，里面可能有空格或右括号，只认最后一个 ')' 才安全；
  // 之后的字段从 3 (state，单字符) 开始，故字段 n 落在 tokens[n - 3]。
  const int closeParen = content.lastIndexOf(')');
  if (closeParen < 0)
  {
    return snapshot;
  }
  const QStringList fields = QString::fromLatin1(content.mid(closeParen + 1))
    .split(QLatin1Char(' '), QString::SkipEmptyParts);
  if (fields.size() <= 21)
  {
    return snapshot;
  }

  bool okUtime = false;
  bool okStime = false;
  bool okRss = false;
  const quint64 utimeTicks = fields.at(11).toULongLong(&okUtime);
  const quint64 stimeTicks = fields.at(12).toULongLong(&okStime);
  const quint64 rssPages = fields.at(21).toULongLong(&okRss);
  if (!okUtime || !okStime || !okRss)
  {
    return snapshot;
  }

  const quint64 ticksPerSecond = positiveSysConf(_SC_CLK_TCK, 100ULL);
  const quint64 pageSize = positiveSysConf(_SC_PAGESIZE, 4096ULL);
  snapshot.cpuTimeMs = (utimeTicks + stimeTicks) * 1000ULL / ticksPerSecond;
  snapshot.memoryBytes = rssPages * pageSize;
  snapshot.valid = snapshot.memoryBytes > 0;
#elif defined(Q_OS_MACOS)
  struct rusage usage;
  if (::getrusage(RUSAGE_SELF, &usage) != 0)
  {
    return snapshot;
  }
  snapshot.cpuTimeMs =
    static_cast<quint64>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000ULL +
    static_cast<quint64>(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000ULL;

  mach_task_basic_info info;
  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
  if (::task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &infoCount) != KERN_SUCCESS)
  {
    return snapshot;
  }
  snapshot.memoryBytes = static_cast<quint64>(info.resident_size);
  snapshot.valid = snapshot.memoryBytes > 0;
#endif

  return snapshot;
}

double bytesToMegabytes(quint64 bytes)
{
  return static_cast<double>(bytes) / kBytesPerMegabyte;
}

CpuResult cpuPercent(const Snapshot& previous, const Snapshot& current, int coreCount)
{
  CpuResult result;
  if (!previous.valid || !current.valid || coreCount <= 0)
  {
    return result;
  }
  if (current.wallTimeMs <= previous.wallTimeMs)
  {
    return result;
  }
  if (current.cpuTimeMs < previous.cpuTimeMs)
  {
    return result;
  }

  const double wallDeltaMs = static_cast<double>(current.wallTimeMs - previous.wallTimeMs);
  const double cpuDeltaMs = static_cast<double>(current.cpuTimeMs - previous.cpuTimeMs);
  const double percent = cpuDeltaMs / (wallDeltaMs * static_cast<double>(coreCount)) * 100.0;
  if (!std::isfinite(percent))
  {
    return result;
  }

  result.valid = true;
  result.percent = percent < 0.0 ? 0.0 : (percent > 100.0 ? 100.0 : percent);
  return result;
}

QString cpuText(const Usage& usage, const Labels& labels)
{
  const bool available = usage.valid && usage.cpuValid &&
                         std::isfinite(usage.cpuPercent) && usage.cpuPercent >= 0.0;
  if (!available)
  {
    return labels.cpu + labels.unavailable;
  }

  const double percent = usage.cpuPercent > 100.0 ? 100.0 : usage.cpuPercent;
  return labels.cpu + fixedOneDecimal(percent) + QStringLiteral("%");
}

QString memoryText(const Usage& usage, const Labels& labels)
{
  const bool available = usage.valid && std::isfinite(usage.memoryMb) &&
                         usage.memoryMb >= 0.0;
  if (!available)
  {
    return labels.memory + labels.unavailable;
  }

  return labels.memory + fixedOneDecimal(usage.memoryMb) + QStringLiteral(" MB");
}

QString availableMemoryText(const Usage& usage, const Labels& labels)
{
  const bool available = usage.availableValid && std::isfinite(usage.availableMemoryMb) &&
                         usage.availableMemoryMb >= 0.0;
  if (!available)
  {
    return labels.available + labels.unavailable;
  }

  return labels.available + fixedOneDecimal(usage.availableMemoryMb) +
         QStringLiteral(" MB");
}

Usage Monitor::update(const Snapshot& snapshot, int coreCount)
{
  Usage usage;
  // 系统可用内存独立于进程指标：进程采集失败的帧照样透传本字段。
  usage.availableValid = snapshot.availableValid;
  usage.availableMemoryMb = bytesToMegabytes(snapshot.availableMemoryBytes);
  if (!snapshot.valid)
  {
    // 采集失败后不能继续拿旧帧当基线，否则恢复时会算出一个虚假的高占用率。
    hasBaseline_ = false;
    return usage;
  }

  usage.valid = true;
  usage.memoryMb = bytesToMegabytes(snapshot.memoryBytes);
  if (hasBaseline_)
  {
    const CpuResult result = cpuPercent(last_, snapshot, coreCount);
    if (result.valid && std::isfinite(result.percent))
    {
      usage.cpuValid = true;
      usage.cpuPercent = result.percent;
    }
  }

  last_ = snapshot;
  hasBaseline_ = true;
  return usage;
}

Usage Monitor::poll(int coreCount)
{
  return update(readSnapshot(), coreCount);
}

void Monitor::reset()
{
  last_ = Snapshot();
  hasBaseline_ = false;
}

}  // namespace CarbonUIProcessStats
