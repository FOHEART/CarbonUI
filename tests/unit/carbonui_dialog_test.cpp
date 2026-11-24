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
 * - 标题条左键按下会进入拖拽状态（以窗口几何变化观察）；
 * - 最小化 / 最大化按钮默认隐藏，可独立开关；最大化按钮切换窗口状态并在
 *   最大化期间把面板圆角归零、字形换成还原形。
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

TEST(CarbonUIDialogTest, WindowButtonHoverUsesTheSharedVisibleColor)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));

    // 悬停底色必须明显亮于标题条底色 #0f0f0f（#2a2a2a 对比过弱，历史坑），
    // 且与 CarbonUIQTitleBar 共用同一常量（见 src/CarbonUIWindowButtonStyle.h）。
    auto* close = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogClose"));
    ASSERT_NE(close, nullptr);
    EXPECT_TRUE(close->styleSheet().contains(QStringLiteral(":hover { background-color: #3a3a3a;")))
        << "hover must use the shared visible color from CarbonUIWindowButtonStyle.h";
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

TEST(CarbonUIDialogTest, WindowButtonsAreHiddenByDefault)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));

    auto* minimize = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogMinimize"));
    auto* maximize = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogMaximize"));
    ASSERT_NE(minimize, nullptr) << "minimize button 'CarbonUIDialogMinimize' is missing";
    ASSERT_NE(maximize, nullptr) << "maximize button 'CarbonUIDialogMaximize' is missing";

    EXPECT_TRUE(minimize->isHidden()) << "minimize button must start hidden";
    EXPECT_TRUE(maximize->isHidden()) << "maximize button must start hidden";
    EXPECT_FALSE(minimize->text().isEmpty()) << "minimize button must show a glyph";
    EXPECT_FALSE(maximize->text().isEmpty()) << "maximize button must show a glyph";
    EXPECT_FALSE(dialog.isMinimizeButtonVisible());
    EXPECT_FALSE(dialog.isMaximizeButtonVisible());
}

TEST(CarbonUIDialogTest, MinimizeAndMaximizeVisibilityAreIndependent)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));

    dialog.setMinimizeButtonVisible(true);
    EXPECT_TRUE(dialog.isMinimizeButtonVisible());
    EXPECT_FALSE(dialog.isMaximizeButtonVisible()) << "enabling minimize must not enable maximize";

    dialog.setMaximizeButtonVisible(true);
    EXPECT_TRUE(dialog.isMinimizeButtonVisible());
    EXPECT_TRUE(dialog.isMaximizeButtonVisible());

    dialog.setMinimizeButtonVisible(false);
    EXPECT_FALSE(dialog.isMinimizeButtonVisible());
    EXPECT_TRUE(dialog.isMaximizeButtonVisible())
        << "disabling minimize must leave maximize untouched";
}

TEST(CarbonUIDialogTest, MaximizeButtonTogglesWindowStateAndGlyph)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    dialog.setMaximizeButtonVisible(true);
    dialog.show();

    auto* maximize = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogMaximize"));
    auto* frame = dialog.findChild<QWidget*>(QStringLiteral("CarbonUIDialogFrame"));
    auto* header = dialog.findChild<QWidget*>(QStringLiteral("CarbonUIDialogHeader"));
    ASSERT_NE(maximize, nullptr);
    ASSERT_NE(frame, nullptr);
    ASSERT_NE(header, nullptr);

    const QString maximizeGlyph = maximize->text();

    maximize->click();
    EXPECT_TRUE(dialog.isMaximized()) << "clicking maximize must maximize the dialog";
    EXPECT_NE(maximize->text(), maximizeGlyph) << "maximize button must switch to a restore glyph";
    EXPECT_TRUE(frame->styleSheet().contains(QStringLiteral("border-radius: 0px")))
        << "a maximized frameless dialog must drop its rounded corners";
    EXPECT_TRUE(header->styleSheet().contains(QStringLiteral("border-top-left-radius: 0px")));

    maximize->click();
    EXPECT_FALSE(dialog.isMaximized()) << "clicking again must restore the normal state";
    EXPECT_EQ(maximize->text(), maximizeGlyph);
    EXPECT_TRUE(frame->styleSheet().contains(QStringLiteral("border-radius: 9px")));
}

TEST(CarbonUIDialogTest, MinimizeButtonMinimizesTheDialog)
{
    CarbonUIDialog dialog(QStringLiteral("ABOUT"));
    dialog.setMinimizeButtonVisible(true);
    dialog.show();

    auto* minimize = dialog.findChild<QPushButton*>(QStringLiteral("CarbonUIDialogMinimize"));
    ASSERT_NE(minimize, nullptr);

    minimize->click();

    EXPECT_TRUE(dialog.isMinimized()) << "clicking minimize must minimize the dialog";
}
