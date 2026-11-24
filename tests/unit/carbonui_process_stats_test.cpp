/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  carbonui_process_stats_test.cpp
 * @brief CarbonUIProcessStats 纯逻辑单元测试：换算、区间占用率、状态机与文案。
 * @ingroup carbonui_tests
 *
 * @details
 * 覆盖策略
 *  - 换算：字节 → MiB 按 1024² 手算，零值与 quint64 上界都不能产生 Inf；
 *  - CPU 百分比：以「100% = 全部逻辑核心满载」为口径，用单核满载、四核四分之一秒、
 *    多核全忙等手算解析解钉住；再覆盖时钟未推进 / 倒退、CPU 时间倒退、
 *    核心数非正、快照无效等乱序与越界输入；
 *  - 采样状态机：首帧有内存但无 CPU 基线，第二帧才出百分比，采集失败要丢弃基线，
 *    整机可用内存独立透传（进程指标失败的帧不受影响）；
 *  - 文案：CPU / 进程内存 / 系统可用内存三种文案的 1 位小数 + 单位、
 *    不可用占位符、NaN / Inf 一律拒绝。
 *
 * 期望值一律手算，不把实现公式抄进用例。平台采集只验证可读到正的常驻内存、
 * 正的系统可用内存与单调时钟，不依赖具体机器负载。
 */

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include <QtCore/QString>

#include "CarbonUIProcessStats.h"

namespace {

/// @brief 浮点断言容差；本模块的期望值都是精确十进制数。
const double kEps = 1e-9;

/// @brief 构造一份有效的进程快照；字段单位见 CarbonUIProcessStats.h。
CarbonUIProcessStats::Snapshot makeSnapshot(quint64 cpuTimeMs, quint64 wallTimeMs,
  quint64 memoryBytes)
{
  CarbonUIProcessStats::Snapshot snapshot;
  snapshot.cpuTimeMs = cpuTimeMs;
  snapshot.wallTimeMs = wallTimeMs;
  snapshot.memoryBytes = memoryBytes;
  snapshot.valid = true;
  return snapshot;
}

/// @brief 纯 ASCII 标签前缀（自带冒号与空格），便于逐字符比对整行文本。
CarbonUIProcessStats::Labels asciiLabels()
{
  CarbonUIProcessStats::Labels labels;
  labels.cpu = QStringLiteral("CPU: ");
  labels.memory = QStringLiteral("MEM: ");
  labels.available = QStringLiteral("AVAIL: ");
  labels.unavailable = QStringLiteral("--");
  return labels;
}

/// @brief 构造一份可直接喂给文本格式化函数的显示结果。
/// @note 可用内存参数带默认值，旧用例无需改动；需要时显式传入。
CarbonUIProcessStats::Usage makeUsage(bool valid, bool cpuValid, double cpuPercent,
  double memoryMb, bool availableValid = false, double availableMemoryMb = 0.0)
{
  CarbonUIProcessStats::Usage usage;
  usage.valid = valid;
  usage.cpuValid = cpuValid;
  usage.cpuPercent = cpuPercent;
  usage.memoryMb = memoryMb;
  usage.availableValid = availableValid;
  usage.availableMemoryMb = availableMemoryMb;
  return usage;
}

}  // namespace

TEST(CarbonUIProcessStats, BytesToMegabytes_UsesBinaryMebi)
{
  // Arrange
  const quint64 bytes = 1572864ULL;

  // Act
  const double megabytes = CarbonUIProcessStats::bytesToMegabytes(bytes);

  // Assert
  EXPECT_NEAR(1.5, megabytes, kEps);
}

TEST(CarbonUIProcessStats, BytesToMegabytes_ZeroIsZero)
{
  // Act
  const double megabytes = CarbonUIProcessStats::bytesToMegabytes(0ULL);

  // Assert
  EXPECT_NEAR(0.0, megabytes, kEps);
}

TEST(CarbonUIProcessStats, BytesToMegabytes_MaxValueStaysFinite)
{
  // Act
  const double megabytes = CarbonUIProcessStats::bytesToMegabytes(
    std::numeric_limits<quint64>::max());

  // Assert
  EXPECT_TRUE(std::isfinite(megabytes));
  EXPECT_GT(megabytes, 0.0);
}

TEST(CarbonUIProcessStats, CpuPercent_FullSingleCoreIsHundred)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 0ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(1000ULL, 1000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 1);

  // Assert
  ASSERT_TRUE(result.valid);
  EXPECT_NEAR(100.0, result.percent, kEps);
}

TEST(CarbonUIProcessStats, CpuPercent_QuarterSecondOnFourCoresIsSixPointTwoFive)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(500ULL, 2000ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(750ULL, 3000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 4);

  // Assert
  ASSERT_TRUE(result.valid);
  EXPECT_NEAR(6.25, result.percent, kEps);
}

TEST(CarbonUIProcessStats, CpuPercent_AllCoresBusyIsHundred)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 0ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(4000ULL, 1000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 4);

  // Assert
  ASSERT_TRUE(result.valid);
  EXPECT_NEAR(100.0, result.percent, kEps);
}

TEST(CarbonUIProcessStats, CpuPercent_ClampsWhenCpuTimeExceedsCapacity)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 0ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(2000ULL, 1000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 1);

  // Assert
  ASSERT_TRUE(result.valid);
  EXPECT_NEAR(100.0, result.percent, kEps);
}

TEST(CarbonUIProcessStats, CpuPercent_RejectsZeroWallDelta)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 1000ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(250ULL, 1000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 4);

  // Assert
  EXPECT_FALSE(result.valid);
}

TEST(CarbonUIProcessStats, CpuPercent_RejectsNonPositiveCoreCount)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 0ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(1000ULL, 1000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult zero =
    CarbonUIProcessStats::cpuPercent(previous, current, 0);
  const CarbonUIProcessStats::CpuResult negative =
    CarbonUIProcessStats::cpuPercent(previous, current, -1);

  // Assert
  EXPECT_FALSE(zero.valid);
  EXPECT_FALSE(negative.valid);
}

TEST(CarbonUIProcessStats, CpuPercent_RejectsCpuTimeGoingBackwards)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(900ULL, 1000ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(800ULL, 2000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 4);

  // Assert
  EXPECT_FALSE(result.valid);
}

TEST(CarbonUIProcessStats, CpuPercent_RejectsWallTimeGoingBackwards)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 3000ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(100ULL, 2000ULL, 0ULL);

  // Act
  const CarbonUIProcessStats::CpuResult result =
    CarbonUIProcessStats::cpuPercent(previous, current, 4);

  // Assert
  EXPECT_FALSE(result.valid);
}

TEST(CarbonUIProcessStats, CpuPercent_RejectsInvalidSnapshotFlag)
{
  // Arrange
  const CarbonUIProcessStats::Snapshot previous = makeSnapshot(0ULL, 0ULL, 0ULL);
  const CarbonUIProcessStats::Snapshot current = makeSnapshot(1000ULL, 1000ULL, 0ULL);
  CarbonUIProcessStats::Snapshot brokenPrevious = previous;
  CarbonUIProcessStats::Snapshot brokenCurrent = current;
  brokenPrevious.valid = false;
  brokenCurrent.valid = false;

  // Act
  const CarbonUIProcessStats::CpuResult fromPrevious =
    CarbonUIProcessStats::cpuPercent(brokenPrevious, current, 1);
  const CarbonUIProcessStats::CpuResult fromCurrent =
    CarbonUIProcessStats::cpuPercent(previous, brokenCurrent, 1);

  // Assert
  EXPECT_FALSE(fromPrevious.valid);
  EXPECT_FALSE(fromCurrent.valid);
}

TEST(CarbonUIProcessStats, Monitor_FirstSampleHasMemoryButNoCpuPercent)
{
  // Arrange
  CarbonUIProcessStats::Monitor monitor;
  const CarbonUIProcessStats::Snapshot first =
    makeSnapshot(100ULL, 1000ULL, 33554432ULL);

  // Act
  const CarbonUIProcessStats::Usage usage = monitor.update(first, 4);

  // Assert
  ASSERT_TRUE(usage.valid);
  EXPECT_FALSE(usage.cpuValid);
  EXPECT_NEAR(32.0, usage.memoryMb, kEps);
}

TEST(CarbonUIProcessStats, Monitor_SecondSampleComputesCpuPercent)
{
  // Arrange
  CarbonUIProcessStats::Monitor monitor;
  monitor.update(makeSnapshot(100ULL, 1000ULL, 33554432ULL), 4);

  // Act
  const CarbonUIProcessStats::Usage usage = monitor.update(
    makeSnapshot(350ULL, 2000ULL, 33554432ULL), 4);

  // Assert
  ASSERT_TRUE(usage.cpuValid);
  EXPECT_NEAR(6.25, usage.cpuPercent, kEps);
}

TEST(CarbonUIProcessStats, Monitor_InvalidSampleClearsBaseline)
{
  // Arrange
  CarbonUIProcessStats::Monitor monitor;
  monitor.update(makeSnapshot(100ULL, 1000ULL, 33554432ULL), 4);
  CarbonUIProcessStats::Snapshot broken;

  // Act
  const CarbonUIProcessStats::Usage failed = monitor.update(broken, 4);
  const CarbonUIProcessStats::Usage recovered = monitor.update(
    makeSnapshot(900ULL, 9000ULL, 33554432ULL), 4);

  // Assert
  EXPECT_FALSE(failed.valid);
  EXPECT_TRUE(recovered.valid);
  EXPECT_FALSE(recovered.cpuValid);
}

TEST(CarbonUIProcessStats, Monitor_CarriesAvailableMemoryWhenProcessSampleInvalid)
{
  // Arrange
  CarbonUIProcessStats::Monitor monitor;
  CarbonUIProcessStats::Snapshot broken = makeSnapshot(0ULL, 0ULL, 0ULL);
  broken.valid = false;
  broken.availableValid = true;
  broken.availableMemoryBytes = 4 * 1024ULL * 1024ULL;  // 4 MiB

  // Act
  const CarbonUIProcessStats::Usage failed = monitor.update(broken, 4);

  // Assert
  EXPECT_FALSE(failed.valid);
  ASSERT_TRUE(failed.availableValid);
  EXPECT_NEAR(4.0, failed.availableMemoryMb, kEps);
}

TEST(CarbonUIProcessStats, Monitor_ResetDropsBaseline)
{
  // Arrange
  CarbonUIProcessStats::Monitor monitor;
  monitor.update(makeSnapshot(100ULL, 1000ULL, 33554432ULL), 4);
  monitor.reset();

  // Act
  const CarbonUIProcessStats::Usage usage = monitor.update(
    makeSnapshot(350ULL, 2000ULL, 33554432ULL), 4);

  // Assert
  EXPECT_FALSE(usage.cpuValid);
}

TEST(CarbonUIProcessStats, CpuText_ShowsOneDecimalAndPercentSign)
{
  // Arrange
  const CarbonUIProcessStats::Usage usage = makeUsage(true, true, 12.34, 100.0);

  // Act
  const QString text = CarbonUIProcessStats::cpuText(usage, asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("CPU: 12.3%"), text);
}

TEST(CarbonUIProcessStats, CpuText_ShowsPlaceholderWhenUnavailable)
{
  // Arrange
  const CarbonUIProcessStats::Usage usage = makeUsage(true, false, 0.0, 100.0);

  // Act
  const QString text = CarbonUIProcessStats::cpuText(usage, asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("CPU: --"), text);
}

TEST(CarbonUIProcessStats, CpuText_RejectsNonFinitePercent)
{
  // Arrange
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  // Act
  const QString fromNan = CarbonUIProcessStats::cpuText(
    makeUsage(true, true, nan, 100.0), asciiLabels());
  const QString fromInf = CarbonUIProcessStats::cpuText(
    makeUsage(true, true, inf, 100.0), asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("CPU: --"), fromNan);
  EXPECT_EQ(QStringLiteral("CPU: --"), fromInf);
}

TEST(CarbonUIProcessStats, MemoryText_ShowsOneDecimalAndMbUnit)
{
  // Arrange
  const CarbonUIProcessStats::Usage usage = makeUsage(true, true, 1.0, 456.78);

  // Act
  const QString text = CarbonUIProcessStats::memoryText(usage, asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("MEM: 456.8 MB"), text);
}

TEST(CarbonUIProcessStats, MemoryText_RejectsNonFiniteMegabytes)
{
  // Arrange
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  // Act
  const QString fromNan = CarbonUIProcessStats::memoryText(
    makeUsage(true, true, 1.0, nan), asciiLabels());
  const QString fromInf = CarbonUIProcessStats::memoryText(
    makeUsage(true, true, 1.0, inf), asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("MEM: --"), fromNan);
  EXPECT_EQ(QStringLiteral("MEM: --"), fromInf);
}

TEST(CarbonUIProcessStats, MemoryText_ShowsPlaceholderWhenSampleInvalid)
{
  // Arrange
  const CarbonUIProcessStats::Usage usage = makeUsage(false, false, 0.0, 999.0);

  // Act
  const QString text = CarbonUIProcessStats::memoryText(usage, asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("MEM: --"), text);
}

TEST(CarbonUIProcessStats, AvailableMemoryText_ShowsOneDecimalAndMbUnit)
{
  // Arrange
  const CarbonUIProcessStats::Usage usage = makeUsage(true, true, 1.0, 100.0,
    true, 8123.42);

  // Act
  const QString text = CarbonUIProcessStats::availableMemoryText(usage, asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("AVAIL: 8123.4 MB"), text);
}

TEST(CarbonUIProcessStats, AvailableMemoryText_ShowsPlaceholderWhenInvalid)
{
  // Arrange
  const CarbonUIProcessStats::Usage usage = makeUsage(true, true, 1.0, 100.0,
    false, 999.0);

  // Act
  const QString text = CarbonUIProcessStats::availableMemoryText(usage, asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("AVAIL: --"), text);
}

TEST(CarbonUIProcessStats, AvailableMemoryText_RejectsNonFiniteMegabytes)
{
  // Arrange
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  // Act
  const QString fromNan = CarbonUIProcessStats::availableMemoryText(
    makeUsage(true, true, 1.0, 100.0, true, nan), asciiLabels());
  const QString fromInf = CarbonUIProcessStats::availableMemoryText(
    makeUsage(true, true, 1.0, 100.0, true, inf), asciiLabels());

  // Assert
  EXPECT_EQ(QStringLiteral("AVAIL: --"), fromNan);
  EXPECT_EQ(QStringLiteral("AVAIL: --"), fromInf);
}

TEST(CarbonUIProcessStats, ReadSnapshot_ReportsPositiveWorkingSetAndMonotonicClock)
{
  // Act
  const CarbonUIProcessStats::Snapshot first = CarbonUIProcessStats::readSnapshot();
  const CarbonUIProcessStats::Snapshot second = CarbonUIProcessStats::readSnapshot();

  // Assert
  ASSERT_TRUE(first.valid);
  ASSERT_TRUE(second.valid);
  EXPECT_GT(first.memoryBytes, 0ULL);
  EXPECT_GT(second.memoryBytes, 0ULL);
  EXPECT_GE(second.wallTimeMs, first.wallTimeMs);

  // 系统可用内存与进程指标独立采集：支持的平台上都应读到正数。
  ASSERT_TRUE(first.availableValid);
  ASSERT_TRUE(second.availableValid);
  EXPECT_GT(first.availableMemoryBytes, 0ULL);
  EXPECT_GT(second.availableMemoryBytes, 0ULL);
}
