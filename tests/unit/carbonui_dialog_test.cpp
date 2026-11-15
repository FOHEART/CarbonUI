/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  carbonui_dialog_test.cpp
 * @brief CarbonUIDialog 行为测试：无边框窗口、标题栏、关闭按钮、信息行与拖拽。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 只断言"契约"，不比对像素（整图外观归 Visual.Screenshot.DialogAbout 基线）：
 * - 窗口标志含 FramelessWindowHint，且是 QDialog；
 * - 标题标签 / 关闭按钮按 objectName 可查，标题可改、可读回；
 * - addInfoRow 生成 "名称: 取值" 行，行数随追加递增；
 * - 关闭按钮走 reject 路径，对话框隐藏；
 * - 标题条左键按下会进入拖拽状态（以窗口几何变化观察）。
 */

#include <gtest/gtest.h>

#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "CarbonUIDialog.h"

TEST(CarbonUIDialogTest, IsFramelessDialog)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));

    EXPECT_TRUE(dialog.windowFlags().testFlag(Qt::FramelessWindowHint))
        << "the dialog must drop the system frame";
    EXPECT_TRUE(dialog.windowFlags().testFlag(Qt::Dialog));
    EXPECT_TRUE(dialog.testAttribute(Qt::WA_TranslucentBackground))
        << "rounded corners need a translucent top-level window";
}

TEST(CarbonUIDialogTest, ExposesTitleAndCloseButton)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));

    auto* title = dialog.findChild<QLabel*>(QStringLiteral("CarbonUIDialogTitle"));
    auto* close = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogClose"));
    ASSERT_NE(title, nullptr) << "title label 'CarbonUIDialogTitle' is missing";
    ASSERT_NE(close, nullptr) << "close button 'CarbonUIDialogClose' is missing";

    EXPECT_EQ(title->text(), QStringLiteral("ABOUT"));
    EXPECT_EQ(dialog.titleText(), QStringLiteral("ABOUT"));
    EXPECT_FALSE(close->text().isEmpty()) << "close button must show a glyph";
}

TEST(CarbonUIDialogTest, TitleTextCanBeUpdated)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    EXPECT_TRUE(dialog.titleText() == QStringLiteral("ABOUT"));

    dialog.setTitleText(QStringLiteral("测试"));

    EXPECT_EQ(dialog.titleText(), QStringLiteral("测试"));
    auto* title = dialog.findChild<QLabel*>(QStringLiteral("CarbonUIDialogTitle"));
    ASSERT_NE(title, nullptr);
    EXPECT_EQ(title->text(), QStringLiteral("测试"));
}

TEST(CarbonUIDialogTest, ClosesThroughRejectOnCloseButtonClick)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    dialog.show();
    ASSERT_TRUE(dialog.isVisible());

    auto* close = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogClose"));
    ASSERT_NE(close, nullptr);
    close->click();

    EXPECT_FALSE(dialog.isVisible()) << "the close button must hide the dialog";
}

TEST(CarbonUIDialogTest, AddInfoRowAppendsPrefixedRows)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    auto* body = dialog.findChild<QWidget*>(QStringLiteral("CarbonUIDialogBody"));
    ASSERT_NE(body, nullptr) << "body widget 'CarbonUIDialogBody' is missing";

    dialog.addInfoRow(QStringLiteral("Program version"), QStringLiteral("2025.2"));
    dialog.addInfoRow(QStringLiteral("License"), QStringLiteral("Perpetual"));

    const QList<QLabel*> rows = body->findChildren<QLabel*>(QStringLiteral("CarbonUIDialogRow"));
    ASSERT_EQ(rows.size(), 2) << "each addInfoRow call must create one label";
    EXPECT_EQ(rows.at(0)->text(), QStringLiteral("Program version: 2025.2"));
    EXPECT_EQ(rows.at(1)->text(), QStringLiteral("License: Perpetual"));
}

TEST(CarbonUIDialogTest, AddInfoRowWithoutValueKeepsSingleText)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    dialog.addInfoRow(QStringLiteral("Built with CarbonUI 0.4.0"));

    auto* body = dialog.findChild<QWidget*>(QStringLiteral("CarbonUIDialogBody"));
    ASSERT_NE(body, nullptr);
    const QList<QLabel*> rows = body->findChildren<QLabel*>(QStringLiteral("CarbonUIDialogRow"));
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(rows.first()->text(), QStringLiteral("Built with CarbonUI 0.4.0"))
        << "the single-argument overload must not add a colon";
}

TEST(CarbonUIDialogTest, DraggingTheHeaderMovesTheDialog)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    dialog.show();
    dialog.move(120, 120);
    const QPoint start = dialog.pos();

    auto* header = dialog.findChild<QWidget*>(QStringLiteral("CarbonUIDialogHeader"));
    ASSERT_NE(header, nullptr);
    const QPoint pressInDialog = header->geometry().center();

    QMouseEvent press(QEvent::MouseButtonPress, pressInDialog,
                      dialog.mapToGlobal(pressInDialog), Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(&dialog, &press);

    QMouseEvent move(QEvent::MouseMove, pressInDialog + QPoint(40, 25),
                     dialog.mapToGlobal(pressInDialog + QPoint(40, 25)), Qt::LeftButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &move);

    EXPECT_EQ(dialog.pos(), start + QPoint(40, 25))
        << "holding the header must drag the frameless window";
}
