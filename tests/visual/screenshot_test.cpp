/**
 * @file  screenshot_test.cpp
 * @brief 截图基线用例：每个界面抓整图，与仓库里的基线图比对，拦截整幅画面的 UI 回归。
 * @author CarbonUI
 * @date  2026-11-06
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 与 `tests/visual/carbonui_render_test.cpp` 的分工：那套用探针像素验证"某几个坐标的
 * 颜色对不对"，本套验证"整幅画面还是不是原样"。后者能发现探针覆盖不到的回归——
 * 间距漂移几个像素、两个控件叠在一起、文字被裁掉一截、某个子控件彻底没画出来。
 *
 * 取景方式与渲染证据一致：所有控件都挂在一块品红底色的父容器上、用绝对坐标摆位，
 * 容器尺寸写死，因此同一平台内每次抓图内容完全一致。文字使用 ASCII，避免把
 * 字体回退链的差异带进基线。
 *
 * 基线生成（首次接入或 UI 有意调整后执行，人工复核再提交）：
 *
 * ```text
 * scripts\test.bat Release Screenshot      # 先跑一遍，缺基线会 skip 并打印路径
 * set CARBONUI_SCREENSHOT_RECORD=1
 * scripts\test.bat Release Screenshot      # 生成/覆盖 tests\visual\baseline\<平台>\*.png
 * set CARBONUI_SCREENSHOT_RECORD=
 * scripts\test.bat Release Screenshot      # 再跑一遍，应当全绿
 * ```
 *
 * 每个用例都遵循同一个节奏：建父容器 → 摆控件 → 交给截图宏 → 宏内抓图、比对、
 * 释放父容器。
 */

#include <gtest/gtest.h>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "CarbonUIClass.h"
#include "groupboxcollapser.h"
#include "screenshot_support.h"

namespace
{
/**
 * @brief 建一块刷成品红底色的父容器，作为截图取景框。
 *
 * @param[in] width  容器宽度，单位像素，取值 [1, 4096]。
 * @param[in] height 容器高度，单位像素，取值 [1, 4096]。
 * @return 新建容器，无父对象，由调用方（截图宏）释放。
 */
QWidget* makeHost(int width, int height)
{
    auto* host = new QWidget();
    host->setObjectName(QStringLiteral("host"));
    host->setStyleSheet(QStringLiteral("QWidget#host { background-color: #ff00ff; }"));
    host->resize(width, height);
    return host;
}
} // namespace

TEST(Screenshot, FormOverview)
{
    // 一块把常用控件摆在一起的"表单"，用来一眼看出整体风格是否走样。
    QWidget* host = makeHost(380, 260);

    auto* title = new QLabel(QStringLiteral("CarbonUI controls"), host);
    title->setGeometry(16, 12, 340, 20);

    auto* button = new QPushButton(QStringLiteral("Confirm"), host);
    button->setGeometry(16, 40, 120, 32);
    CarbonUIClass::applyButtonStyle(button);

    // applyCheckStyle 是"纯指示灯"式复选框（会清掉文字），所以旁边的说明文字用 QLabel。
    auto* checkbox = new QCheckBox(host);
    checkbox->setGeometry(16, 84, 16, 16);
    CarbonUIClass::applyCheckStyle(checkbox);
    checkbox->setChecked(true);

    auto* checkboxLabel = new QLabel(QStringLiteral("Remember me"), host);
    checkboxLabel->setGeometry(40, 84, 160, 16);

    auto* radio = new QRadioButton(QStringLiteral("Mode A"), host);
    radio->setGeometry(16, 116, 120, 28);
    CarbonUIClass::applyRadioStyle(radio, CarbonUIClass::Radio_FourRound, 120);
    radio->setChecked(true);

    auto* edit = new QLineEdit(QStringLiteral("42.5"), host);
    edit->setGeometry(16, 152, 180, 28);
    CarbonUIClass::applyLineEditStyle_numberInput(edit);

    auto* combo = new QComboBox(host);
    combo->setGeometry(16, 188, 180, 28);
    combo->addItem(QStringLiteral("Item one"));
    combo->addItem(QStringLiteral("Item two"));
    CarbonUIClass::setupComboBoxStyle(combo);

    // 标签样式放最后统一刷：该函数只处理"当时已存在"的子控件。
    CarbonUIClass::setWidgetLabelsStyleRecursively(host);

    CARBONUI_CHECK_SCREENSHOT(host, "form-overview");
}

TEST(Screenshot, ButtonRoundMatchesBaseline)
{
    QWidget* host = makeHost(160, 72);
    auto* button = new QPushButton(QStringLiteral("OK"), host);
    button->setGeometry(20, 20, 120, 32);
    CarbonUIClass::applyButtonStyle(button);

    CARBONUI_CHECK_SCREENSHOT(host, "button-round");
}

TEST(Screenshot, ButtonRectangleMatchesBaseline)
{
    QWidget* host = makeHost(160, 72);
    auto* button = new QPushButton(QStringLiteral("OK"), host);
    button->setGeometry(20, 20, 120, 32);
    CarbonUIClass::applyButtonStyle(button, CarbonUIClass::Shape_Rectangle);

    CARBONUI_CHECK_SCREENSHOT(host, "button-rectangle");
}

TEST(Screenshot, ButtonDisabledMatchesBaseline)
{
    // 禁用态是最容易被样式表漏掉的分支：颜色应整体变灰，且不再有强调色填充。
    QWidget* host = makeHost(160, 72);
    auto* button = new QPushButton(QStringLiteral("OK"), host);
    button->setGeometry(20, 20, 120, 32);
    CarbonUIClass::applyButtonStyle(button);
    button->setEnabled(false);

    CARBONUI_CHECK_SCREENSHOT(host, "button-disabled");
}

TEST(Screenshot, RadioSegmentsMatchBaseline)
{
    // 左圆 + 中直 + 右圆拼成一条分段选择器，整体形状最容易在样式改动后退化。
    auto* left = new QRadioButton(QStringLiteral("L"));
    auto* middle = new QRadioButton(QStringLiteral("M"));
    auto* right = new QRadioButton(QStringLiteral("R"));
    CarbonUIClass::applyRadioStyle(left, CarbonUIClass::Radio_LeftRound, 60);
    CarbonUIClass::applyRadioStyle(middle, CarbonUIClass::Radio_Rectangle, 60);
    CarbonUIClass::applyRadioStyle(right, CarbonUIClass::Radio_RightRound, 60);
    left->setAutoExclusive(false);
    middle->setAutoExclusive(false);
    right->setAutoExclusive(false);

    const int segmentWidth = left->minimumSizeHint().width();
    const int segmentHeight = qMax(28, left->minimumSizeHint().height());
    ASSERT_GT(segmentWidth, 0);

    const int originX = 20;
    const int originY = 16;
    QWidget* host = makeHost(originX * 2 + segmentWidth * 3, originY * 2 + segmentHeight);

    left->setParent(host);
    middle->setParent(host);
    right->setParent(host);
    left->setGeometry(originX, originY, segmentWidth, segmentHeight);
    middle->setGeometry(originX + segmentWidth, originY, segmentWidth, segmentHeight);
    right->setGeometry(originX + segmentWidth * 2, originY, segmentWidth, segmentHeight);
    left->setChecked(true);
    middle->setChecked(true);
    right->setChecked(true);

    CARBONUI_CHECK_SCREENSHOT(host, "radio-segments");
}

TEST(Screenshot, CheckboxStatesMatchBaseline)
{
    // 同一张图里放未勾选与已勾选两枚指示灯，颜色对比与描边粗细一并对齐基线。
    QWidget* host = makeHost(96, 48);

    auto* plain = new QCheckBox(host);
    plain->setGeometry(16, 16, 16, 16);
    CarbonUIClass::applyCheckStyle(plain);

    auto* checked = new QCheckBox(host);
    checked->setGeometry(56, 16, 16, 16);
    CarbonUIClass::applyCheckStyle(checked);
    checked->setChecked(true);

    CARBONUI_CHECK_SCREENSHOT(host, "checkbox-states");
}

TEST(Screenshot, LineEditNumberMatchesBaseline)
{
    QWidget* host = makeHost(220, 60);
    auto* edit = new QLineEdit(QStringLiteral("42.5"), host);
    edit->setGeometry(20, 16, 180, 28);
    CarbonUIClass::applyLineEditStyle_numberInput(edit);

    CARBONUI_CHECK_SCREENSHOT(host, "line-edit-number");
}

TEST(Screenshot, TabWidgetMatchesBaseline)
{
    QWidget* host = makeHost(260, 140);
    auto* tabs = new QTabWidget(host);
    tabs->setGeometry(10, 10, 240, 120);
    tabs->addTab(new QLabel(QStringLiteral("page one")), QStringLiteral("One"));
    tabs->addTab(new QLabel(QStringLiteral("page two")), QStringLiteral("Two"));
    CarbonUIClass::setupTabWidgetStyle(tabs);

    CARBONUI_CHECK_SCREENSHOT(host, "tab-widget");
}

TEST(Screenshot, TreeWidgetMatchesBaseline)
{
    QWidget* host = makeHost(260, 150);
    auto* tree = new QTreeWidget(host);
    tree->setGeometry(10, 10, 240, 120);
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList() << QStringLiteral("Name") << QStringLiteral("Value"));
    for (int i = 0; i < 3; ++i)
    {
        auto* item = new QTreeWidgetItem(tree);
        item->setText(0, QStringLiteral("row %1").arg(i));
        item->setText(1, QStringLiteral("v%1").arg(i));
    }
    CarbonUIClass::applyTreeWidgetStyle(tree);
    tree->setCurrentItem(tree->topLevelItem(1));

    CARBONUI_CHECK_SCREENSHOT(host, "tree-widget");
}

TEST(Screenshot, GroupBoxExpandedMatchesBaseline)
{
    QWidget* host = makeHost(260, 170);
    auto* box = new QGroupBox(QStringLiteral("Advanced"), host);
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(new QLabel(QStringLiteral("body text")));
    auto* action = new QPushButton(QStringLiteral("Action"));
    CarbonUIClass::applyButtonStyle(action);
    layout->addWidget(action);
    QGroupBoxCollapser::installIntoGroupBox(box, nullptr, true);
    box->setGeometry(10, 10, 240, box->sizeHint().height());

    CARBONUI_CHECK_SCREENSHOT(host, "groupbox-expanded");
}

TEST(Screenshot, GroupBoxCollapsedMatchesBaseline)
{
    QWidget* host = makeHost(260, 170);
    auto* box = new QGroupBox(QStringLiteral("Advanced"), host);
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(new QLabel(QStringLiteral("body text")));
    auto* action = new QPushButton(QStringLiteral("Action"));
    CarbonUIClass::applyButtonStyle(action);
    layout->addWidget(action);
    QGroupBoxCollapser::installIntoGroupBox(box, nullptr, false);
    box->setGeometry(10, 10, 240, box->sizeHint().height());

    CARBONUI_CHECK_SCREENSHOT(host, "groupbox-collapsed");
}
