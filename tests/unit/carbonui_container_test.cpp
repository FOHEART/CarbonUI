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
#include <QtWidgets/QMainWindow>
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
#include "CarbonUIQTitleBar.h"
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
    ASSERT_NE(area.verticalScrollBar(), nullptr);
    ASSERT_NE(area.horizontalScrollBar(), nullptr);
    // 滚动条外观统一委托给 CarbonUIQScrollBar：样式表写在滚动条对象自身，
    // 优先级高于 QScrollArea 的 "QScrollArea QWidget" 通配规则
    EXPECT_QSS_CONTAINS(area.verticalScrollBar()->styleSheet(),
                        "QScrollBar:vertical { background: #2e2e2e; width: 8px;");
    EXPECT_QSS_CONTAINS(area.horizontalScrollBar()->styleSheet(),
                        "QScrollBar:horizontal { background: #2e2e2e; height: 8px;");
    EXPECT_EQ(area.verticalScrollBar()->minimumWidth(), 0)
        << "thickness must come from QSS so :hover can grow it, not from setMinimumWidth";
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

TEST(MenuBarStyle, ApplyMenuStyleDarkensPopupsOutsideTheBar)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQMenuBar::applyMenuStyle(nullptr));

    QMenu menu;

    CarbonUIQMenuBar::applyMenuStyle(&menu);

    EXPECT_EQ(menu.font().pointSize(), 10);
    EXPECT_TRUE(menu.font().bold());
    EXPECT_QSS_CONTAINS(menu.styleSheet(), "QMenu { background: #2e2e2e;");
    EXPECT_QSS_CONTAINS(menu.styleSheet(), "QMenu::item:selected { background: #3b3b3b;");
}

TEST(MenuBarStyle, AdoptExistingMenusMigratesActionsAndDarkensPopups)
{
    QMainWindow window;
    auto* oldBar = window.menuBar();
    oldBar->setObjectName("designerMenuBar");
    auto* fileMenu = oldBar->addMenu("File");
    auto* helpMenu = oldBar->addMenu("Help");

    QMenuBar* newBar = CarbonUIQMenuBar::adoptExistingMenus(&window);

    ASSERT_NE(newBar, nullptr);
    EXPECT_NE(newBar, oldBar);
    EXPECT_EQ(newBar->objectName(), QStringLiteral("designerMenuBar"));
    ASSERT_EQ(newBar->actions().size(), 2);
    EXPECT_EQ(newBar->actions().at(0)->menu(), fileMenu);
    EXPECT_EQ(newBar->actions().at(1)->menu(), helpMenu);
    // 菜单被重新挂到 window 下后已不在旧栏样式表作用域内，必须单独刷外观。
    EXPECT_QSS_CONTAINS(fileMenu->styleSheet(), "QMenu { background: #2e2e2e;");
    EXPECT_QSS_CONTAINS(helpMenu->styleSheet(), "QMenu { background: #2e2e2e;");
    EXPECT_EQ(CarbonUIQMenuBar::adoptExistingMenus(nullptr), nullptr);
}

TEST(MenuBarStyle, UnifyFontsCoversNestedSubmenusAndItems)
{
    QMainWindow window;
    QMenuBar* bar = window.menuBar();
    CarbonUIQMenuBar::applyStyle(bar);
    const QFont barFont = bar->font();

    // 模拟 .ui 混设字号：子菜单显式 14pt、菜单项显式 12pt；动态菜单不设字体。
    QMenu* top = bar->addMenu("Top");
    top->addAction("Item");
    QMenu* sub = top->addMenu("Sub");
    sub->addAction("SubItem");
    QFont odd; odd.setPointSize(14); sub->setFont(odd);
    QFont oddItem; oddItem.setPointSize(12); top->actions().at(0)->setFont(oddItem);
    // 动态菜单以窗口为父、仅通过 action 可达（主程序「视图」子菜单的场景）。
    auto* detached = new QMenu("Detached", &window);
    detached->addAction("DetachedItem");
    top->insertMenu(top->actions().at(0), detached);

    CarbonUIQMenuBar::unifyFonts(bar);

    EXPECT_EQ(top->font().pointSize(), barFont.pointSize());
    EXPECT_TRUE(top->font().bold());
    // 子菜单与菜单项都被显式覆盖，不依赖 Qt 的父到子字体传播。
    EXPECT_EQ(sub->font().pointSize(), barFont.pointSize());
    EXPECT_TRUE(sub->font().bold());
    EXPECT_EQ(top->actions().at(0)->font().pointSize(), barFont.pointSize());
    EXPECT_EQ(sub->actions().at(0)->font().pointSize(), barFont.pointSize());
    EXPECT_EQ(detached->font().pointSize(), barFont.pointSize());
    EXPECT_EQ(detached->actions().at(0)->font().pointSize(), barFont.pointSize());
    EXPECT_NO_FATAL_FAILURE(CarbonUIQMenuBar::unifyFonts(nullptr));
}

TEST(TitleBarStyle, FixesTwentySixPixelBarWithWindowButtons)
{
    QWidget window;
    CarbonUIQTitleBar bar(&window);

    EXPECT_EQ(bar.height(), 26);
    EXPECT_EQ(bar.objectName(), QStringLiteral("CarbonUIQTitleBar"));
    // QWidget 子类必须开 WA_StyledBackground，否则 QSS 的 #0f0f0f 标题条底色
    // 不会绘制，透出宿主的 #212121，窗口按钮悬停反馈会几乎看不见。
    EXPECT_TRUE(bar.testAttribute(Qt::WA_StyledBackground));
    EXPECT_NE(bar.findChild<QPushButton*>("CarbonUIQTitleBarMinimize"), nullptr);
    EXPECT_NE(bar.findChild<QPushButton*>("CarbonUIQTitleBarMaximize"), nullptr);
    EXPECT_NE(bar.findChild<QPushButton*>("CarbonUIQTitleBarClose"), nullptr);
    EXPECT_QSS_CONTAINS(bar.styleSheet(), "border-top-left-radius: 0px;");
}

TEST(TitleBarStyle, CornerRadiusAndTitleFollowTheWindow)
{
    QWidget window;
    window.setWindowTitle("MotionGlove");
    CarbonUIQTitleBar bar(&window);

    bar.setCornerRadius(9);
    EXPECT_QSS_CONTAINS(bar.styleSheet(), "border-top-left-radius: 9px;");
    // 标题文字自动跟随窗口标题
    EXPECT_EQ(bar.titleText(), QStringLiteral("MotionGlove"));
    bar.setTitleText("Manual");
    EXPECT_EQ(bar.titleText(), QStringLiteral("Manual"));

    bar.setLogoPixmap(QPixmap());
    QLabel* logo = bar.findChild<QLabel*>("CarbonUIQTitleBarLogo");
    ASSERT_NE(logo, nullptr);
    EXPECT_TRUE(logo->pixmap() == nullptr || logo->pixmap()->isNull());
}

TEST(TitleBarStyle, MaximizedWindowZeroesRadiusAndSwapsGlyph)
{
    QWidget window;
    CarbonUIQTitleBar bar(&window);
    bar.setCornerRadius(9);

    window.setWindowState(Qt::WindowMaximized);
    bar.syncToWindowState();

    EXPECT_QSS_CONTAINS(bar.styleSheet(), "border-top-left-radius: 0px;");
    QPushButton* maximize = bar.findChild<QPushButton*>("CarbonUIQTitleBarMaximize");
    ASSERT_NE(maximize, nullptr);
    EXPECT_EQ(maximize->text(), QString::fromUtf8("\xE2\x9D\x90"));

    window.setWindowState(Qt::WindowNoState);
    bar.syncToWindowState();
    EXPECT_QSS_CONTAINS(bar.styleSheet(), "border-top-left-radius: 9px;");
}

TEST(TitleBarStyle, WindowButtonHoverUsesTheSharedVisibleColor)
{
    QWidget window;
    CarbonUIQTitleBar bar(&window);

    // 悬停底色必须明显亮于标题条底色 #0f0f0f（#2a2a2a 对比过弱，历史坑），
    // 且与 CarbonUIDialog 共用同一常量（见 src/CarbonUIWindowButtonStyle.h）。
    QPushButton* close = bar.findChild<QPushButton*>("CarbonUIQTitleBarClose");
    ASSERT_NE(close, nullptr);
    EXPECT_QSS_CONTAINS(close->styleSheet(), ":hover { background-color: #3a3a3a;");
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
