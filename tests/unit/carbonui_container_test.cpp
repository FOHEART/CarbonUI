/**
 * @file  carbonui_container_test.cpp
 * @brief 容器类控件样式助手的行为用例（滚动区 / 菜单栏 / 工具栏 / 标签页 /
 *        下拉框 / 状态栏 / 递归标签 / 全局 Tooltip）。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 这些助手除了写 QSS 还会改动几何、字体与折叠标志，调用方（主程序窗口）
 * 依赖这些"顺带"的副作用，因此用例断言的是可观测的控件状态；
 * 每个助手都补一条空指针用例，保证主程序在控件缺失时不会崩溃。
 */

#include <gtest/gtest.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "CarbonUIClass.h"
#include "CarbonUIQComboBox.h"
#include "CarbonUIQMenuBar.h"
#include "CarbonUIQScrollArea.h"
#include "CarbonUIQStatusBar.h"
#include "CarbonUIQTabWidget.h"
#include "CarbonUIQToolBar.h"
#include "qt_test_support.h"

TEST(ScrollAreaStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQScrollArea::applyStyle(nullptr));
}

TEST(ScrollAreaStyle, SlimScrollBarAndBorderlessFrame)
{
    QScrollArea area;

    CarbonUIQScrollArea::applyStyle(&area);

    EXPECT_QSS_CONTAINS(area.styleSheet(), "QScrollArea { border: none; }");
    EXPECT_QSS_CONTAINS(area.styleSheet(), "QScrollBar:vertical");
    ASSERT_NE(area.verticalScrollBar(), nullptr);
    EXPECT_EQ(area.verticalScrollBar()->minimumWidth(), 4);
}

TEST(MenuBarStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQMenuBar::applyStyle(nullptr));
}

TEST(MenuBarStyle, UsesInWindowBarWithBoldTenPointFont)
{
    QMenuBar bar;

    CarbonUIQMenuBar::applyStyle(&bar);

    EXPECT_FALSE(bar.isNativeMenuBar());
    EXPECT_EQ(bar.minimumHeight(), 26);
    EXPECT_EQ(bar.maximumHeight(), 26);
    EXPECT_EQ(bar.font().pointSize(), 10);
    EXPECT_TRUE(bar.font().bold());
    EXPECT_QSS_CONTAINS(bar.styleSheet(), "QMenuBar { background: #3b3b3b;");
    EXPECT_QSS_CONTAINS(bar.styleSheet(), "QMenu::item:selected");
}

TEST(ToolBarStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQToolBar::applyStyle(nullptr));
}

TEST(ToolBarStyle, LocksToolBarAndAppliesIconMetrics)
{
    QToolBar toolBar;

    CarbonUIQToolBar::applyStyle(&toolBar);

    EXPECT_FALSE(toolBar.isMovable());
    EXPECT_FALSE(toolBar.isFloatable());
    // setIconSize() clears the explicit minimum set by setFixedHeight(), so the
    // height is pinned through the maximum only - that is the observable contract.
    EXPECT_EQ(toolBar.maximumHeight(), 32);
    EXPECT_EQ(toolBar.iconSize(), QSize(28, 28));
    EXPECT_EQ(toolBar.contentsMargins(), QMargins(32, 0, 1, 0));
    ASSERT_NE(toolBar.layout(), nullptr);
    EXPECT_EQ(toolBar.layout()->spacing(), 2);
    EXPECT_QSS_CONTAINS(toolBar.styleSheet(), "QToolButton:hover { border: 1px solid #5d5d5d; }");
}

TEST(TabWidgetStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQTabWidget::applyStyle(nullptr));
}

TEST(TabWidgetStyle, StylesPaneAndTabBar)
{
    QTabWidget tabs;

    CarbonUIQTabWidget::applyStyle(&tabs);

    EXPECT_QSS_CONTAINS(tabs.styleSheet(), "QTabWidget::pane");
    EXPECT_QSS_CONTAINS(tabs.styleSheet(), "QTabBar::tab:selected");
    EXPECT_QSS_CONTAINS(tabs.styleSheet(), "padding-right: 66px");
}

TEST(ComboBoxStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQComboBox::applyStyle(nullptr));
}

TEST(ComboBoxStyle, UsesRoundedFieldAndResourceArrow)
{
    QComboBox combo;

    CarbonUIQComboBox::applyStyle(&combo);

    EXPECT_QSS_CONTAINS(combo.styleSheet(), "border-radius: 12px");
    EXPECT_QSS_CONTAINS(combo.styleSheet(), "QComboBox::drop-down");
    // carbonui_rcc.qrc maps prefix "/img", see the documented resource convention
    EXPECT_QSS_CONTAINS(combo.styleSheet(), ":/img/Resources/img/arrow_down.png");
}

TEST(StatusBarStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQStatusBar::applyStyle(nullptr));
}

TEST(StatusBarStyle, UsesMutedSurfaceColours)
{
    QStatusBar bar;

    CarbonUIQStatusBar::applyStyle(&bar);

    EXPECT_QSS_CONTAINS(bar.styleSheet(), "background-color: #2e2e2e");
    EXPECT_QSS_CONTAINS(bar.styleSheet(), "color: #676767");
}

TEST(LabelStyleRecursion, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIClass::setWidgetLabelsStyleRecursively(nullptr));
}

TEST(LabelStyleRecursion, RecoloursLabelsAtEveryDepthAndLeavesOthersAlone)
{
    QWidget root;
    auto* directLabel = new QLabel(QStringLiteral("direct"), &root);
    directLabel->setStyleSheet(QStringLiteral("color: red;"));
    auto* nested = new QWidget(&root);
    auto* nestedLayout = new QVBoxLayout(nested);
    auto* deepLabel = new QLabel(QStringLiteral("deep"), nested);
    nestedLayout->addWidget(deepLabel);
    auto* button = new QPushButton(QStringLiteral("go"), &root);

    CarbonUIClass::setWidgetLabelsStyleRecursively(&root);

    EXPECT_QSS_CONTAINS(directLabel->styleSheet(), "#676767");
    EXPECT_QSS_NOT_CONTAINS(directLabel->styleSheet(), "red");
    EXPECT_QSS_CONTAINS(deepLabel->styleSheet(), "#676767");
    EXPECT_TRUE(button->styleSheet().isEmpty()) << "non-label widgets must not be restyled";
}

TEST(TooltipStyle, AppendsRuleToApplicationStyleSheet)
{
    const QString original = qApp->styleSheet();
    qApp->setStyleSheet(QStringLiteral("QPushButton { color: red; }"));

    CarbonUIClass::applyTooltipStyle();
    const QString styled = qApp->styleSheet();

    EXPECT_EQ(carbonui_test::countOccurrences(styled, "QToolTip {"), 1);
    EXPECT_QSS_CONTAINS(styled, "background-color: #0f0f0f");
    EXPECT_QSS_CONTAINS(styled, "color: #029de8");
    EXPECT_QSS_CONTAINS(styled, "QPushButton { color: red; }")
        << "existing application style must be preserved";

    qApp->setStyleSheet(original);
}
