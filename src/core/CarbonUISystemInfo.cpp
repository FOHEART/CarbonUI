/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUISystemInfo.cpp
 * @brief CarbonUISystemInfo 的实现：跨平台采集系统信息并按语言渲染。
 * @ingroup carbonui
 *
 * @details
 * collectSystemInfo() 各子系统在 Windows 下优先使用注册表 / SetupAPI /
 * kernel32，失败则回退到 wmic；Linux/macOS 下读取 /proc、调用
 * lspci/free/df/system_profiler 等命令。所有命令调用均带超时（2~3 秒）。
 * renderSystemInfo() 只做文案拼接，不含任何平台代码。
 */

#include "CarbonUISystemInfo.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QSysInfo>
#include <QtCore/QTextStream>
#include <QtCore/QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winreg.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "ole32.lib")
#endif

namespace CarbonUISystemInfo
{

namespace
{

/// @brief "zh" 开头的语言代码一律视为中文（zh_CN / zh_TW / zh-Hans 等）。
bool isChinese(const QString& languageCode)
{
  return languageCode.startsWith(QStringLiteral("zh"),
                                 Qt::CaseInsensitive);
}

#ifdef Q_OS_WIN
/// @brief CPU 型号（注册表 ProcessorNameString）与物理核心数（wmic）。
void collectCpuInfo(SystemInfoData& data)
{
  // 注册表获取 CPU 型号（比 wmic 快且可靠）。
  HKEY hKey = NULL;
  const LONG openResult = ::RegOpenKeyExA(
      HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
      0, KEY_READ, &hKey);
  if (openResult == ERROR_SUCCESS)
  {
    char processorName[256] = { 0 };
    DWORD bufferSize = sizeof(processorName);
    if (::RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL,
                           reinterpret_cast<LPBYTE>(processorName),
                           &bufferSize) == ERROR_SUCCESS)
    {
      data.cpuModel = QString::fromLocal8Bit(processorName);
    }
    ::RegCloseKey(hKey);
  }

  // 逻辑核心数（SYSTEM_INFO；Wow64 进程下是宿主核数，与任务管理器一致）。
  SYSTEM_INFO sysInfo;
  ::GetSystemInfo(&sysInfo);
  data.logicalCores = static_cast<int>(sysInfo.dwNumberOfProcessors);

  // 物理核心数（wmic 2 秒超时；失败保持 0，渲染时不输出该行）。
  QProcess process;
  process.start(QStringLiteral("wmic"),
                QStringList() << QStringLiteral("cpu")
                              << QStringLiteral("get")
                              << QStringLiteral("NumberOfCores"));
  if (process.waitForFinished(2000))
  {
    const QStringList lines =
        QString::fromLocal8Bit(process.readAllStandardOutput())
            .split(QLatin1Char('\n'));
    if (lines.size() > 1)
    {
      bool ok = false;
      const int cores = lines.at(1).trimmed().toInt(&ok);
      if (ok && cores > 0)
      {
        data.physicalCores = cores;
      }
    }
  }
}

/// @brief 实际安装物理内存：GetPhysicallyInstalledSystemMemory（SMBIOS）
///        优先，GlobalMemoryStatusEx（应用可见内存）回退。
void collectMemoryInfo(SystemInfoData& data)
{
  // GetPhysicallyInstalledSystemMemory 读取 SMBIOS 记录的装机容量（单位 KB），
  // 不受固件保留 / 核显共享显存占用影响，与「系统属性 → 关于」一致。
  typedef BOOL(WINAPI * PGetPhysicallyInstalledSystemMemory)(PULONGLONG);
  const HMODULE kernel32 = ::GetModuleHandleA("kernel32.dll");
  if (kernel32 != NULL)
  {
    const PGetPhysicallyInstalledSystemMemory getInstalledMemory =
        reinterpret_cast<PGetPhysicallyInstalledSystemMemory>(
            ::GetProcAddress(kernel32, "GetPhysicallyInstalledSystemMemory"));
    if (getInstalledMemory != NULL)
    {
      ULONGLONG totalMemKB = 0;
      if (getInstalledMemory(&totalMemKB) && totalMemKB > 0)
      {
        data.totalMemoryGB =
            totalMemKB * 1024.0 / (1024.0 * 1024.0 * 1024.0);
        data.memoryValid = true;
        return;
      }
    }
  }

  // 回退：ullTotalPhys 是应用程序可见的物理内存，已扣除固件保留区域，
  // 可能比实际安装容量略小。
  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(memStatus);
  if (::GlobalMemoryStatusEx(&memStatus) && memStatus.ullTotalPhys > 0)
  {
    data.totalMemoryGB =
        static_cast<double>(memStatus.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
    data.memoryValid = true;
  }
}

/// @brief 显卡友好名称：SetupAPI 枚举显示设备，注册表 DriverDesc 回退。
void collectGpuInfo(SystemInfoData& data)
{
  const HDEVINFO devInfo = ::SetupDiGetClassDevs(
      &GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
  if (devInfo != INVALID_HANDLE_VALUE)
  {
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(devInfoData);
    for (DWORD i = 0; ::SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); ++i)
    {
      char deviceName[256] = { 0 };
      if (::SetupDiGetDeviceRegistryPropertyA(
              devInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL,
              reinterpret_cast<PBYTE>(deviceName), sizeof(deviceName), NULL))
      {
        data.gpuNames.append(QString::fromLocal8Bit(deviceName));
      }
    }
    ::SetupDiDestroyDeviceInfoList(devInfo);
  }

  // SetupAPI 未命中时按显示类 GUID 遍历注册表子键的 DriverDesc。
  if (data.gpuNames.isEmpty())
  {
    HKEY hKey = NULL;
    const LONG openResult = ::RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Class\\"
        "{4D36E968-E325-11CE-BFC1-08002BE10318}",
        0, KEY_READ, &hKey);
    if (openResult == ERROR_SUCCESS)
    {
      for (DWORD i = 0;; ++i)
      {
        char subKeyName[256] = { 0 };
        DWORD subKeyNameSize = sizeof(subKeyName);
        if (::RegEnumKeyExA(hKey, i, subKeyName, &subKeyNameSize, NULL, NULL,
                            NULL, NULL) != ERROR_SUCCESS)
        {
          break;
        }
        HKEY hSubKey = NULL;
        if (::RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &hSubKey)
            == ERROR_SUCCESS)
        {
          char deviceName[256] = { 0 };
          DWORD bufferSize = sizeof(deviceName);
          if (::RegQueryValueExA(hSubKey, "DriverDesc", NULL, NULL,
                                 reinterpret_cast<LPBYTE>(deviceName),
                                 &bufferSize) == ERROR_SUCCESS)
          {
            data.gpuNames.append(QString::fromLocal8Bit(deviceName));
          }
          ::RegCloseKey(hSubKey);
        }
      }
      ::RegCloseKey(hKey);
    }
  }
}

/// @brief 存储概要：GetLogicalDrives 的盘符串。
void collectStorageInfo(SystemInfoData& data)
{
  const DWORD drives = ::GetLogicalDrives();
  QString driveList;
  for (int i = 0; i < 26; ++i)
  {
    if (drives & (1u << i))
    {
      driveList += QString(QLatin1Char('A' + i)) + QLatin1Char(' ');
    }
  }
  if (!driveList.isEmpty())
  {
    data.storageSummary = driveList;
    data.storageValid = true;
  }
}
#endif  // Q_OS_WIN

#if !defined(Q_OS_WIN)
/// @brief CPU 型号（/proc/cpuinfo 的 model name 行）与逻辑核心数。
/// @note procfs 文件 stat 大小恒为 0，QFile/QTextStream 的 atEnd() 在首次
///       读到数据之前恒为 true，不能逐行循环读；统一 readAll() 后按行拆分。
void collectCpuInfo(SystemInfoData& data)
{
  QFile cpuinfoFile(QStringLiteral("/proc/cpuinfo"));
  if (cpuinfoFile.open(QIODevice::ReadOnly))
  {
    const QStringList lines =
        QString::fromLatin1(cpuinfoFile.readAll()).split(QLatin1Char('\n'));
    for (const QString& line : lines)
    {
      if (line.contains(QStringLiteral("model name"), Qt::CaseInsensitive))
      {
        // 行形如 "model name\t: Intel ..."，取冒号后的型号部分。
        const int colonIndex = line.indexOf(QLatin1Char(':'));
        if (colonIndex >= 0)
        {
          data.cpuModel = line.mid(colonIndex + 1).trimmed();
        }
        break;
      }
    }
    cpuinfoFile.close();
  }
  data.logicalCores = QThread::idealThreadCount();
}

/// @brief 物理内存（GB）：/proc/meminfo 的 MemTotal 优先，free/system_profiler 回退。
void collectMemoryInfo(SystemInfoData& data)
{
  QFile memFile(QStringLiteral("/proc/meminfo"));
  if (memFile.open(QIODevice::ReadOnly))
  {
    const QStringList lines =
        QString::fromLatin1(memFile.readAll()).split(QLatin1Char('\n'));
    for (const QString& line : lines)
    {
      if (line.startsWith(QStringLiteral("MemTotal"), Qt::CaseInsensitive))
      {
        const QStringList parts = line.split(QRegExp("\\s+"));
        if (parts.size() >= 2)
        {
          bool ok = false;
          const long long memKB = parts.at(1).toLongLong(&ok);
          if (ok && memKB > 0)
          {
            data.totalMemoryGB = memKB / (1024.0 * 1024.0);
            data.memoryValid = true;
          }
        }
        break;
      }
    }
    memFile.close();
    if (data.memoryValid)
    {
      return;
    }
  }

  // Linux 回退：free -b 的 total 列。
  QProcess freeProcess;
  freeProcess.start(QStringLiteral("free"), QStringList() << QStringLiteral("-b"));
  if (freeProcess.waitForFinished(2000))
  {
    const QStringList lines =
        QString::fromLocal8Bit(freeProcess.readAllStandardOutput())
            .split(QLatin1Char('\n'));
    if (lines.size() >= 2)
    {
      const QStringList parts = lines.at(1).split(QRegExp("\\s+"));
      if (parts.size() >= 2)
      {
        bool ok = false;
        const long long memBytes = parts.at(1).toLongLong(&ok);
        if (ok && memBytes > 0)
        {
          data.totalMemoryGB = memBytes / (1024.0 * 1024.0 * 1024.0);
          data.memoryValid = true;
          return;
        }
      }
    }
  }

  // macOS 回退：system_profiler 的 "Memory:" 行（无法得到纯 GB 数值，
  // 原样作为概要展示）。
  QProcess profiler;
  profiler.start(QStringLiteral("system_profiler"),
                 QStringList() << QStringLiteral("SPMemoryDataType"));
  if (profiler.waitForFinished(3000))
  {
    const QStringList lines =
        QString::fromLocal8Bit(profiler.readAllStandardOutput())
            .split(QLatin1Char('\n'));
    for (const QString& line : lines)
    {
      if (line.contains(QStringLiteral("Memory:"), Qt::CaseInsensitive))
      {
        data.memoryValid = false;  // 数值口径不统一，只把原文塞进存储概要展示
        data.storageSummary = line.trimmed();
        data.storageValid = true;
        return;
      }
    }
  }
}

/// @brief 显卡信息：lspci -nn 里含 VGA / 3D 的行。
void collectGpuInfo(SystemInfoData& data)
{
  QProcess process;
  process.start(QStringLiteral("lspci"), QStringList() << QStringLiteral("-nn"));
  process.waitForFinished(3000);
  const QStringList lines =
      QString::fromLocal8Bit(process.readAllStandardOutput())
          .split(QLatin1Char('\n'));
  for (const QString& line : lines)
  {
    if (line.contains(QStringLiteral("VGA"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("3D"), Qt::CaseInsensitive))
    {
      data.gpuNames.append(line);
    }
  }
}

/// @brief 存储概要：df -h / 的数据行。
void collectStorageInfo(SystemInfoData& data)
{
  QProcess process;
  process.start(QStringLiteral("df"), QStringList() << QStringLiteral("-h")
                                                    << QStringLiteral("/"));
  if (process.waitForFinished(2000))
  {
    const QStringList lines =
        QString::fromLocal8Bit(process.readAllStandardOutput())
            .split(QLatin1Char('\n'));
    if (lines.size() > 1 && !lines.at(1).trimmed().isEmpty())
    {
      data.storageSummary = lines.at(1);
      data.storageValid = true;
    }
  }
}
#endif  // !Q_OS_WIN

}  // namespace

bool isChineseLanguageCode(const QString& languageCode)
{
  return isChinese(languageCode);
}

Labels labelsForLanguage(const QString& languageCode)
{
  Labels labels;
  if (isChinese(languageCode))
  {
    labels.osSection = QStringLiteral("操作系统");
    labels.cpuSection = QStringLiteral("CPU信息");
    labels.memorySection = QStringLiteral("内存信息");
    labels.gpuSection = QStringLiteral("显卡信息");
    labels.storageSection = QStringLiteral("存储信息");

    labels.osName = QStringLiteral("操作系统");
    labels.kernelVersion = QStringLiteral("内核版本");
    labels.architecture = QStringLiteral("架构");
    labels.cpuModel = QStringLiteral("CPU型号");
    labels.logicalCores = QStringLiteral("逻辑核心数");
    labels.physicalCores = QStringLiteral("物理核心数");
    labels.totalMemory = QStringLiteral("总物理内存");
    labels.gpuName = QStringLiteral("显卡");
    labels.detectedDrives = QStringLiteral("检测到驱动器");

    labels.cpuUnavailable = QStringLiteral("CPU信息: 无法获取详细信息");
    labels.memoryUnavailable = QStringLiteral("无法获取内存信息");
    labels.gpuUnavailable = QStringLiteral("无法获取显卡信息");
    labels.storageUnavailable = QStringLiteral("无法获取存储信息");
  }
  else
  {
    labels.osSection = QStringLiteral("Operating System");
    labels.cpuSection = QStringLiteral("CPU Information");
    labels.memorySection = QStringLiteral("Memory Information");
    labels.gpuSection = QStringLiteral("GPU Information");
    labels.storageSection = QStringLiteral("Storage Information");

    labels.osName = QStringLiteral("Operating System");
    labels.kernelVersion = QStringLiteral("Kernel Version");
    labels.architecture = QStringLiteral("Architecture");
    labels.cpuModel = QStringLiteral("CPU Model");
    labels.logicalCores = QStringLiteral("Logical Cores");
    labels.physicalCores = QStringLiteral("Physical Cores");
    labels.totalMemory = QStringLiteral("Total Physical Memory");
    labels.gpuName = QStringLiteral("GPU");
    labels.detectedDrives = QStringLiteral("Detected Drives");

    labels.cpuUnavailable = QStringLiteral("CPU info: details unavailable");
    labels.memoryUnavailable = QStringLiteral("Unable to retrieve memory info");
    labels.gpuUnavailable = QStringLiteral("Unable to retrieve GPU info");
    labels.storageUnavailable = QStringLiteral("Unable to retrieve storage info");
  }
  return labels;
}

SystemInfoData collectSystemInfo()
{
  SystemInfoData data;
  data.osType = QSysInfo::productType();
  data.kernelVersion = QSysInfo::kernelVersion();
  data.cpuArchitecture = QSysInfo::currentCpuArchitecture();

  // 各子系统独立采集：一个失败不影响其余。
  collectCpuInfo(data);
  collectMemoryInfo(data);
  collectGpuInfo(data);
  collectStorageInfo(data);
  return data;
}

QString renderSystemInfo(const SystemInfoData& data, const Labels& labels)
{
  QString text;

  // 操作系统：QSysInfo 三个字段恒可用。
  text += QStringLiteral("=== %1 ===\n").arg(labels.osSection);
  text += QStringLiteral("%1: %2\n").arg(labels.osName, data.osType);
  text += QStringLiteral("%1: %2\n").arg(labels.kernelVersion, data.kernelVersion);
  text += QStringLiteral("%1: %2\n").arg(labels.architecture, data.cpuArchitecture);
  text += QLatin1Char('\n');

  // CPU：型号 / 逻辑核心数 / 物理核心数任一有效即输出；全空显示占位提示。
  text += QStringLiteral("=== %1 ===\n").arg(labels.cpuSection);
  if (data.cpuModel.isEmpty() && data.logicalCores <= 0)
  {
    text += labels.cpuUnavailable + QLatin1Char('\n');
  }
  else
  {
    if (!data.cpuModel.isEmpty())
    {
      text += QStringLiteral("%1: %2\n").arg(labels.cpuModel, data.cpuModel);
    }
    if (data.logicalCores > 0)
    {
      text += QStringLiteral("%1: %2\n")
                  .arg(labels.logicalCores)
                  .arg(data.logicalCores);
    }
    if (data.physicalCores > 0)
    {
      text += QStringLiteral("%1: %2\n")
                  .arg(labels.physicalCores)
                  .arg(data.physicalCores);
    }
  }
  text += QLatin1Char('\n');

  // 内存：容量无效时显示占位提示，有效时保留两位小数。
  text += QStringLiteral("=== %1 ===\n").arg(labels.memorySection);
  if (!data.memoryValid || !qIsFinite(data.totalMemoryGB)
      || data.totalMemoryGB <= 0.0)
  {
    text += labels.memoryUnavailable + QLatin1Char('\n');
  }
  else
  {
    text += QStringLiteral("%1: %2 GB\n")
                .arg(labels.totalMemory)
                .arg(data.totalMemoryGB, 0, 'f', 2);
  }
  text += QLatin1Char('\n');

  // 显卡：逐台输出友好名称；空列表显示占位提示。
  text += QStringLiteral("=== %1 ===\n").arg(labels.gpuSection);
  if (data.gpuNames.isEmpty())
  {
    text += labels.gpuUnavailable + QLatin1Char('\n');
  }
  else
  {
    for (const QString& gpu : data.gpuNames)
    {
      text += QStringLiteral("%1: %2\n").arg(labels.gpuName, gpu);
    }
  }
  text += QLatin1Char('\n');

  // 存储：Windows 盘符串 / Unix 根分区 df 行；无效时显示占位提示。
  text += QStringLiteral("=== %1 ===\n").arg(labels.storageSection);
  if (!data.storageValid || data.storageSummary.isEmpty())
  {
    text += labels.storageUnavailable + QLatin1Char('\n');
  }
  else
  {
    text += QStringLiteral("%1: %2\n").arg(labels.detectedDrives,
                                           data.storageSummary);
  }

  return text;
}

}  // namespace CarbonUISystemInfo
