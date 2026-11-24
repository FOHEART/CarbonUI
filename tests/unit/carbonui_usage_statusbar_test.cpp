/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  carbonui_usage_statusbar_test.cpp
 * @brief CarbonUIUsageStatusBar 单元测试：语言切换、轮播顺序与自定义消息。
 * @ingroup carbonui_tests
 *
 * @details
 * 覆盖策略（offscreen QPA，无窗口渲染）：
 *  - 语言切换：setLanguage("zh"/"en") 后默认消息与读数标签文案同步重建；
 *  - 轮播：经 QMetaObject::invokeMethod 同步驱动 switchUsageDisplay，
 *    断言 CPU → 进程内存 → 系统可用内存的顺序与各项目当前语言标签；
 *  - 自定义消息：setMessage 后语言切换不覆盖；传空串恢复默认消息。
 *
 * 平台采集在本套件 carbonui_process_stats_test.cpp 已验证，这里只验证
 * 控件自身的装配与切换逻辑，不依赖具体机器负载。
 */

#include <gtest/gtest.h>

#include <QtCore/QMetaObject>
#include <QtWidgets/QLabel>

#include "CarbonUIUsageStatusBar.h"

namespace {

/// @brief 找到右侧读数标签：唯一一个右对齐的 QLabel。
QLabel* findUsageLabel(const CarbonUIUsageStatusBar& bar)
{
  const QList<QLabel*> labels = bar.findChildren<QLabel*>();
  for (QLabel* label : labels)
  {
    if (label->alignment().testFlag(Qt::AlignRight))
    {
      return label;
    }
  }
  return nullptr;
}

/// @brief 找到左侧消息标签：非右对齐（默认左对齐）的 QLabel。
QLabel* findMessageLabel(const CarbonUIUsageStatusBar& bar)
{
  const QList<QLabel*> labels = bar.findChildren<QLabel*>();
  for (QLabel* label : labels)
  {
    if (!label->alignment().testFlag(Qt::AlignRight))
    {
      return label;
    }
  }
  return nullptr;
}

/// @brief 用 invokeMethod 同步驱动私有槽，免去真实定时器等待。
void invokePrivateSlot(CarbonUIUsageStatusBar& bar, const char* slot)
{
  ASSERT_TRUE(QMetaObject::invokeMethod(&bar, slot, Qt::DirectConnection));
}

}  // namespace

TEST(CarbonUIUsageStatusBar, LanguageSwitch_UpdatesDefaultMessageAndReadout)
{
  // Arrange
  CarbonUIUsageStatusBar bar;
  invokePrivateSlot(bar, "sampleUsage");
  QLabel* const usage = findUsageLabel(bar);
  QLabel* const message = findMessageLabel(bar);
  ASSERT_NE(usage, nullptr);
  ASSERT_NE(message, nullptr);

  // Act + Assert：中文
  bar.setLanguage(QStringLiteral("zh"));
  EXPECT_EQ(QStringLiteral("zh"), bar.language());
  EXPECT_EQ(QStringLiteral("就绪"), bar.defaultMessage());
  EXPECT_EQ(QStringLiteral("就绪"), message->text());
  EXPECT_TRUE(usage->text().startsWith(QStringLiteral("CPU占用：")));

  // Act + Assert：英文
  bar.setLanguage(QStringLiteral("en"));
  EXPECT_EQ(QStringLiteral("en"), bar.language());
  EXPECT_EQ(QStringLiteral("Ready"), bar.defaultMessage());
  EXPECT_EQ(QStringLiteral("Ready"), message->text());
  EXPECT_TRUE(usage->text().startsWith(QStringLiteral("CPU Usage:")));
  EXPECT_FALSE(usage->text().contains(QStringLiteral("占用")));
}

TEST(CarbonUIUsageStatusBar, Cycle_RotatesCpuMemoryAvailable)
{
  // Arrange
  CarbonUIUsageStatusBar bar;
  invokePrivateSlot(bar, "sampleUsage");
  QLabel* const usage = findUsageLabel(bar);
  ASSERT_NE(usage, nullptr);
  bar.setLanguage(QStringLiteral("zh"));
  invokePrivateSlot(bar, "switchUsageDisplay");  // 0 → 1

  // Act + Assert：项目 1 = 进程内存
  EXPECT_TRUE(usage->text().startsWith(QStringLiteral("内存占用：")));
  invokePrivateSlot(bar, "switchUsageDisplay");  // 1 → 2

  // Act + Assert：项目 2 = 系统可用内存
  EXPECT_TRUE(usage->text().startsWith(QStringLiteral("可用物理内存：")));
  invokePrivateSlot(bar, "switchUsageDisplay");  // 2 → 0

  // Act + Assert：回到项目 0 = CPU
  EXPECT_TRUE(usage->text().startsWith(QStringLiteral("CPU占用：")));
}

TEST(CarbonUIUsageStatusBar, CustomMessage_SurvivesLanguageSwitchAndClearRestores)
{
  // Arrange
  CarbonUIUsageStatusBar bar;
  QLabel* const message = findMessageLabel(bar);
  ASSERT_NE(message, nullptr);

  // Act
  bar.setMessage(QStringLiteral("Syncing device..."));

  // Assert：自定义消息显示且语言切换不覆盖
  EXPECT_EQ(QStringLiteral("Syncing device..."), message->text());
  bar.setLanguage(QStringLiteral("zh"));
  EXPECT_EQ(QStringLiteral("Syncing device..."), message->text());

  // Act + Assert：空串恢复当前语言默认消息
  bar.setMessage(QString());
  EXPECT_EQ(QStringLiteral("就绪"), message->text());
}
