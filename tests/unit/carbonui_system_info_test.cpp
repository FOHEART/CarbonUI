/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  carbonui_system_info_test.cpp
 * @brief CarbonUISystemInfo / CarbonUISystemInfoDialog 单元测试。
 * @ingroup carbonui_tests
 *
 * @details
 * 覆盖策略（offscreen QPA，无窗口渲染）：
 *  - 内置双语：labelsForLanguage 按 "zh" 前缀取中文，其余英文；
 *  - 渲染：伪造 SystemInfoData 断言分节标题、字段行、两位小数内存、
 *    物理核心数缺省行省略与各分节占位提示；
 *  - 对话框：offscreen 构造后用 QMetaObject::invokeMethod 同步驱动
 *    setLanguage，断言标题 / 按钮 / 正文文案整体切换；copyToClipboard
 *    写入剪贴板。
 * 真实平台采集只做存在性冒烟（本机 Windows 下 SetupAPI / kernel32 路径
 * 应可用），具体数值不跨机器稳定，不作精确断言。
 */

#include <gtest/gtest.h>

#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

#include "CarbonUISystemInfo.h"
#include "CarbonUISystemInfoDialog.h"

namespace
{

/// @brief 构造一份字段齐全的采集数据，供渲染用例伪造输入。
CarbonUISystemInfo::SystemInfoData makeFullData()
{
  CarbonUISystemInfo::SystemInfoData data;
  data.osType = QStringLiteral("windows");
  data.kernelVersion = QStringLiteral("10.0.26100");
  data.cpuArchitecture = QStringLiteral("x86_64");
  data.cpuModel = QStringLiteral("Fake CPU 1234");
  data.logicalCores = 16;
  data.physicalCores = 8;
  data.totalMemoryGB = 32.0;
  data.memoryValid = true;
  data.gpuNames << QStringLiteral("Fake GPU A") << QStringLiteral("Fake GPU B");
  data.storageSummary = QStringLiteral("C D E ");
  data.storageValid = true;
  return data;
}

}  // namespace

TEST(CarbonUISystemInfo, LabelsForLanguage_ZhPrefixGivesChineseOthersEnglish)
{
  // Act + Assert：zh / zh_CN 都是中文。
  const CarbonUISystemInfo::Labels zh =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("zh"));
  EXPECT_EQ(QStringLiteral("操作系统"), zh.osSection);
  EXPECT_EQ(QStringLiteral("总物理内存"), zh.totalMemory);

  const CarbonUISystemInfo::Labels zhCn =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("zh_CN"));
  EXPECT_EQ(QStringLiteral("内存信息"), zhCn.memorySection);

  // Act + Assert：en / 其他代码一律英文。
  const CarbonUISystemInfo::Labels en =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("en"));
  EXPECT_EQ(QStringLiteral("Operating System"), en.osSection);
  EXPECT_EQ(QStringLiteral("Total Physical Memory"), en.totalMemory);

  const CarbonUISystemInfo::Labels fr =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("fr_FR"));
  EXPECT_EQ(QStringLiteral("GPU Information"), fr.gpuSection);
}

TEST(CarbonUISystemInfo, RenderSystemInfo_ChineseSectionsFieldsAndMemoryFormat)
{
  // Arrange
  const CarbonUISystemInfo::Labels labels =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("zh"));

  // Act
  const QString text =
      CarbonUISystemInfo::renderSystemInfo(makeFullData(), labels);

  // Assert：五个分节齐全，字段与数值格式正确。
  EXPECT_TRUE(text.contains(QStringLiteral("=== 操作系统 ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("操作系统: windows")));
  EXPECT_TRUE(text.contains(QStringLiteral("内核版本: 10.0.26100")));
  EXPECT_TRUE(text.contains(QStringLiteral("架构: x86_64")));
  EXPECT_TRUE(text.contains(QStringLiteral("=== CPU信息 ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("CPU型号: Fake CPU 1234")));
  EXPECT_TRUE(text.contains(QStringLiteral("逻辑核心数: 16")));
  EXPECT_TRUE(text.contains(QStringLiteral("物理核心数: 8")));
  EXPECT_TRUE(text.contains(QStringLiteral("=== 内存信息 ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("总物理内存: 32.00 GB")));
  EXPECT_TRUE(text.contains(QStringLiteral("=== 显卡信息 ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("显卡: Fake GPU A")));
  EXPECT_TRUE(text.contains(QStringLiteral("显卡: Fake GPU B")));
  EXPECT_TRUE(text.contains(QStringLiteral("=== 存储信息 ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("检测到驱动器: C D E")));
}

TEST(CarbonUISystemInfo, RenderSystemInfo_EnglishSwitchesAllWording)
{
  // Arrange
  const CarbonUISystemInfo::Labels labels =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("en"));

  // Act
  const QString text =
      CarbonUISystemInfo::renderSystemInfo(makeFullData(), labels);

  // Assert：英文分节与字段；不得残留中文。
  EXPECT_TRUE(text.contains(QStringLiteral("=== Operating System ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("Operating System: windows")));
  EXPECT_TRUE(text.contains(QStringLiteral("Kernel Version: 10.0.26100")));
  EXPECT_TRUE(text.contains(QStringLiteral("CPU Model: Fake CPU 1234")));
  EXPECT_TRUE(text.contains(QStringLiteral("Total Physical Memory: 32.00 GB")));
  EXPECT_TRUE(text.contains(QStringLiteral("=== GPU Information ===")));
  EXPECT_TRUE(text.contains(QStringLiteral("GPU: Fake GPU A")));
  EXPECT_TRUE(text.contains(QStringLiteral("=== Storage Information ===")));
  EXPECT_FALSE(text.contains(QChar(0x7CFB)));  // 「系」字，任何中文字符都不该出现
}

TEST(CarbonUISystemInfo, RenderSystemInfo_OmitsUnknownPhysicalCoresAndShowsPlaceholders)
{
  // Arrange：全部字段失效。
  CarbonUISystemInfo::SystemInfoData empty;

  // Act + Assert：CPU 分节整段占位、无「物理核心数」行。
  const CarbonUISystemInfo::Labels zh =
      CarbonUISystemInfo::labelsForLanguage(QStringLiteral("zh"));
  const QString zhText = CarbonUISystemInfo::renderSystemInfo(empty, zh);
  EXPECT_TRUE(zhText.contains(QStringLiteral("CPU信息: 无法获取详细信息")));
  EXPECT_FALSE(zhText.contains(QStringLiteral("物理核心数")));
  EXPECT_TRUE(zhText.contains(QStringLiteral("无法获取内存信息")));
  EXPECT_TRUE(zhText.contains(QStringLiteral("无法获取显卡信息")));
  EXPECT_TRUE(zhText.contains(QStringLiteral("无法获取存储信息")));

  // Act + Assert：型号未知但逻辑核心数已知时，仍输出核心数行。
  CarbonUISystemInfo::SystemInfoData partial;
  partial.logicalCores = 8;
  const QString partialText =
      CarbonUISystemInfo::renderSystemInfo(partial, zh);
  EXPECT_TRUE(partialText.contains(QStringLiteral("逻辑核心数: 8")));
  EXPECT_FALSE(partialText.contains(QStringLiteral("CPU型号")));
}

TEST(CarbonUISystemInfo, CollectSystemInfo_RealPlatformSmoke)
{
  // Act：本机真实采集（Windows 下走 SetupAPI / kernel32 路径）。
  const CarbonUISystemInfo::SystemInfoData data =
      CarbonUISystemInfo::collectSystemInfo();

  // Assert：QSysInfo 字段恒可用；支持平台上核心数 / 内存 / 存储应为有效值。
  EXPECT_FALSE(data.osType.isEmpty());
  EXPECT_FALSE(data.kernelVersion.isEmpty());
  EXPECT_FALSE(data.cpuArchitecture.isEmpty());
#if defined(Q_OS_WIN)
  EXPECT_GT(data.logicalCores, 0);
  EXPECT_TRUE(data.memoryValid);
  EXPECT_GT(data.totalMemoryGB, 0.0);
  EXPECT_TRUE(data.storageValid);
#endif
#if defined(Q_OS_LINUX)
  // Linux 采集路径回归护栏：/proc/cpuinfo 与 /proc/meminfo 必须解析成功
  // （CI 曾因 procfs stat size 为 0 导致 atEnd() 首查即真而全部读空）。
  EXPECT_GT(data.logicalCores, 0);
  EXPECT_TRUE(data.memoryValid);
  EXPECT_GT(data.totalMemoryGB, 0.0);
  EXPECT_FALSE(data.cpuModel.isEmpty());
#endif
}

TEST(CarbonUISystemInfoDialog, LanguageSwitch_RebuildsTitleButtonsAndBody)
{
  // Arrange：offscreen 构造对话框（构造时完成一次真实采集）。
  CarbonUISystemInfoDialog dialog;
  ASSERT_NE(dialog.findChild<QGroupBox*>(), nullptr);
  ASSERT_NE(dialog.findChild<QTextEdit*>(), nullptr);

  // Act + Assert：英文。
  ASSERT_TRUE(QMetaObject::invokeMethod(&dialog, "setLanguage",
                                        Q_ARG(QString, QStringLiteral("en"))));
  EXPECT_EQ(QStringLiteral("System Information"), dialog.windowTitle());
  const QString enText = dialog.findChild<QTextEdit*>()->toPlainText();
  EXPECT_TRUE(enText.contains(QStringLiteral("=== Operating System ===")));
  EXPECT_FALSE(enText.contains(QChar(0x7CFB)));

  // Act + Assert：中文。
  ASSERT_TRUE(QMetaObject::invokeMethod(&dialog, "setLanguage",
                                        Q_ARG(QString, QStringLiteral("zh"))));
  EXPECT_EQ(QStringLiteral("系统信息"), dialog.windowTitle());
  const QString zhText = dialog.findChild<QTextEdit*>()->toPlainText();
  EXPECT_TRUE(zhText.contains(QStringLiteral("=== 操作系统 ===")));
  EXPECT_TRUE(zhText.contains(QStringLiteral("=== 存储信息 ===")));
}

TEST(CarbonUISystemInfoDialog, CopyToClipboard_PutsBodyTextIntoClipboard)
{
  // Arrange
  CarbonUISystemInfoDialog dialog;
  ASSERT_TRUE(QMetaObject::invokeMethod(&dialog, "setLanguage",
                                        Q_ARG(QString, QStringLiteral("zh"))));

  // Act
  ASSERT_TRUE(QMetaObject::invokeMethod(&dialog, "copyToClipboard"));

  // Assert：剪贴板内容与正文一致且为当前语言。
  const QString clipboard = QApplication::clipboard()->text();
  EXPECT_EQ(dialog.findChild<QTextEdit*>()->toPlainText(), clipboard);
  EXPECT_TRUE(clipboard.contains(QStringLiteral("=== 操作系统 ===")));
}
