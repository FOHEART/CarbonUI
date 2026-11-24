/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIProcessStats.h
 * @brief 当前进程 CPU 占用率 / 常驻内存与整机可用内存的采集、换算及状态栏文案格式化。
 * @author CarbonUI
 * @date  2026-11-24
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 模块边界：只依赖 Qt5::Core 值类型与标准库，不引用 QWidget / OpenGL，
 * 因此既能被 CarbonUIUsageStatusBar 在 GUI 线程每秒调用，也能在无窗口环境
 * 逐值断言（见 tests/unit/carbonui_process_stats_test.cpp）。
 *
 * 口径约定
 *  - CPU 百分比按「全部逻辑核心归一化」：100% 表示占满整机 CPU，与 Windows
 *    任务管理器一致。单核 25% 占用在 4 核机器上显示 6.25%。调用方传入
 *    QThread::idealThreadCount()，本模块只做纯计算，不查询硬件。
 *  - 进程内存取常驻集：Windows 为工作集 WorkingSetSize，Linux 为
 *    /proc/self/stat 的 rss，macOS 为 mach task 的 resident_size。
 *  - 整机可用内存：Windows 取 GlobalMemoryStatusEx 的 ullAvailPhys；Linux 取
 *    /proc/meminfo 的 MemAvailable；macOS 取 (free + inactive) 页数 × 页大小。
 *    与进程指标相互独立标记（availableValid），一者失败不影响另一者。
 *  - 显示单位 MB 按 1 MB = 1024² 字节（即 MiB），与任务管理器一致。
 *
 * 采样方式：readSnapshot() 读取一次累计 CPU 时间与内存读数，Monitor 保存上一帧，
 * 用相邻两帧的差值除以「墙钟差 × 核心数」得到区间占用率。首帧没有基线，
 * cpuValid 为 false，界面应显示占位符而不是 0.0%。
 *
 * 平台差异：Windows 用 GetProcessTimes / GetProcessMemoryInfo /
 * GlobalMemoryStatusEx，Linux 解析 /proc/self/stat 与 /proc/meminfo，macOS 用
 * getrusage / task_info / host_statistics64；不支持的平台返回 valid=false。
 * 墙钟一律取 std::chrono::steady_clock，避免系统时间被调整时出现负区间。
 *
 * 线程安全：Monitor 实例有内部状态，只应在单个线程（GUI 线程）内顺序调用；
 * 其余函数均为无状态的纯函数。错误处理：不抛异常，非法 / 乱序输入返回
 * valid=false，非有限值一律拒绝并显示占位符。
 */

#ifndef CARBONUIPROCESSSTATS_H
#define CARBONUIPROCESSSTATS_H

#include "carbonui_global.h"

#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace CarbonUIProcessStats {

/**
 * @brief 一次资源采样的原始读数（进程指标 + 整机可用内存）。
 */
struct Snapshot
{
  quint64 cpuTimeMs = 0;    ///< 进程累计 CPU 时间（用户态 + 内核态），单位毫秒。
  quint64 memoryBytes = 0;  ///< 进程常驻内存，单位字节。
  quint64 wallTimeMs = 0;   ///< 单调时钟读数，单位毫秒。
  bool valid = false;       ///< 进程指标采集是否成功；false 时前三个字段无意义。

  /// @brief 整机可用物理内存（未被占用、可分配给新申请的部分），单位字节。
  /// @details Windows 取 GlobalMemoryStatusEx 的 ullAvailPhys；Linux 取
  ///          /proc/meminfo 的 MemAvailable；macOS 取 (free + inactive) 页数 × 页大小。
  quint64 availableMemoryBytes = 0;

  /// @brief 整机可用内存采集是否成功；与 @ref valid 相互独立。
  /// @details 进程指标失败时系统内存仍可能可用（反之亦然），两者分开标记。
  bool availableValid = false;
};

/**
 * @brief 相邻两帧算出的区间 CPU 占用率。
 *
 * @details 用 valid 区分「算不出来」与「真的是 0%」：空闲进程连续两帧的
 * CPU 时间可能完全相同，此时 percent 为 0 但 valid 必须为 true。
 */
struct CpuResult
{
  bool valid = false;    ///< 区间是否有效；false 时 percent 无意义。
  double percent = 0.0;  ///< CPU 占用率，取值 [0, 100]。
};

/**
 * @brief 可直接交给状态栏显示的一次结果。
 */
struct Usage
{
  bool valid = false;             ///< 本帧采集是否成功；false 时进程内存与 CPU 都显示占位符。
  bool cpuValid = false;          ///< 是否有可用的 CPU 基线（首帧为 false）。
  double cpuPercent = 0.0;        ///< CPU 占用率，取值 [0, 100]。
  double memoryMb = 0.0;          ///< 进程常驻内存，单位 MB（1024² 字节）。

  bool availableValid = false;     ///< 整机可用内存采集是否成功；独立于 @ref valid。
  double availableMemoryMb = 0.0;  ///< 整机可用物理内存，单位 MB（1024² 字节）。
};

/**
 * @brief 状态栏文案前缀，由调用方按当前语言取好词后传入。
 *
 * @details 与 CarbonUI 其余控件的纯逻辑约定一致：文案不写死在本模块里，
 * 格式化逻辑才能保持可单测。CarbonUIUsageStatusBar 内置中英文映射，
 * 其他宿主可按自己的语言接口填充 Labels。
 *
 * 前缀需自带冒号 / 空格等分隔（中文用全角冒号，英文用半角冒号加空格），
 * 格式化函数直接把数值拼在前缀之后，不再额外插入分隔符。
 */
struct Labels
{
  QString cpu;           ///< CPU 字段前缀，例如 "CPU Usage: "（含冒号）。
  QString memory;        ///< 进程内存字段前缀，例如 "Memory Usage: "。
  QString available;     ///< 系统可用内存字段前缀，例如 "Available Physical Memory: "。
  QString unavailable;   ///< 不可用占位符，例如 "--"。
};

/**
 * @brief 读取当前进程与整机的一次资源快照。
 *
 * @return 进程指标采集成功时 valid=true 且 memoryBytes>0；失败时返回全零且
 *         valid=false。整机可用内存独立标记在 availableValid / availableMemoryBytes。
 * @note 复杂度 O(1)；Linux 分支只读 /proc/self/stat 与 /proc/meminfo。
 */
Snapshot readSnapshot();

/**
 * @brief 字节换算成 MB（1 MB = 1024² 字节）。
 *
 * @param[in] bytes 字节数，允许 0 与 quint64 上界。
 * @return 对应的 MB 数，始终为有限非负数。
 */
double bytesToMegabytes(quint64 bytes);

/**
 * @brief 由相邻两帧计算区间 CPU 占用率。
 *
 * @details 计算式为「CPU 时间增量 / (墙钟增量 × 核心数) × 100」，
 * 结果夹到 [0, 100]；核心数非正、快照无效、时钟未推进 / 倒退、
 * CPU 累计时间倒退都返回 valid=false。
 *
 * @param[in] previous 上一帧快照。
 * @param[in] current  当前帧快照。
 * @param[in] coreCount 逻辑核心数，必须 > 0。
 * @return 区间占用率；输入非法时 valid=false、percent=0。
 */
CpuResult cpuPercent(const Snapshot& previous, const Snapshot& current, int coreCount);

/**
 * @brief 生成 CPU 字段的状态栏文案。
 *
 * @param[in] usage  最近一次采样结果；cpuValid=false 或非有限值显示占位符。
 * @param[in] labels 当前语言前缀。
 * @return 形如 "CPU Usage: 12.3%" 或 "CPU Usage: --" 的文本。
 */
QString cpuText(const Usage& usage, const Labels& labels);

/**
 * @brief 生成进程内存字段的状态栏文案。
 *
 * @param[in] usage  最近一次采样结果；valid=false 或非有限值显示占位符。
 * @param[in] labels 当前语言前缀。
 * @return 形如 "Memory Usage: 456.8 MB" 或 "Memory Usage: --" 的文本。
 */
QString memoryText(const Usage& usage, const Labels& labels);

/**
 * @brief 生成整机可用内存字段的状态栏文案。
 *
 * @param[in] usage  最近一次采样结果；availableValid=false 或非有限值显示占位符。
 * @param[in] labels 当前语言前缀。
 * @return 形如 "Available Physical Memory: 8123.4 MB" 或 "Available Physical Memory: --" 的文本。
 */
QString availableMemoryText(const Usage& usage, const Labels& labels);

/**
 * @brief 资源指标采样状态机，保存上一帧以计算区间占用率。
 *
 * @details 调用方每秒喂一帧即可；采集失败的帧会清掉 CPU 基线，避免用
 * 过期快照算出虚假的高占用率。整机可用内存独立透传，不受进程指标失败影响。
 * 仅 GUI 线程使用，非线程安全。
 */
class Monitor
{
public:
  /**
   * @brief 喂入一帧快照并得到可显示结果。
   * @param[in] snapshot 平台采集的快照，可注入以便单元测试。
   * @param[in] coreCount 逻辑核心数，必须 > 0。
   * @return 本帧结果；首帧或恢复后的首帧 cpuValid=false。
   */
  Usage update(const Snapshot& snapshot, int coreCount);

  /**
   * @brief 现场采集一帧并更新状态，等价于 update(readSnapshot(), coreCount)。
   * @param[in] coreCount 逻辑核心数，必须 > 0。
   * @return 本帧结果。
   */
  Usage poll(int coreCount);

  /** @brief 丢弃基线，下一次 update 重新按首帧处理。 */
  void reset();

private:
  Snapshot last_;                 ///< 上一帧有效快照。
  bool hasBaseline_ = false;      ///< 是否已有可比较的上一帧。
};

}  // namespace CarbonUIProcessStats

#endif  // CARBONUIPROCESSSTATS_H
