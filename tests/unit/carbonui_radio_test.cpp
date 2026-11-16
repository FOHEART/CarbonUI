/**
 * @file  carbonui_radio_test.cpp
 * @brief `CarbonUIQRadioButton::applyStyle()` 的行为用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 单选按钮的视觉完全由 QSS + 子 QLabel 拼出来：原生 indicator 被压成 0px，
 * 文字搬进 `<objectName>_label`，再由一个事件过滤器在 paint/resize 时把
 * `text()` 同步回去。用例覆盖：形状映射、最小宽度、文字搬迁、重复调用不重复
 * 建控件、以及"改 text() 后同步到标签"这条隐式约定。
 */

#include <gtest/gtest.h>

#include <QtCore/QEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>

#include "CarbonUIQRadioButton.h"
#include "qt_test_support.h"

namespace
{
QLabel* centreLabel(QRadioButton* button)
{
    const QString name = button->objectName() + QStringLiteral("_label");
    return button->findChild<QLabel*>(name, Qt::FindDirectChildrenOnly);
}
} // namespace

TEST(RadioStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQRadioButton::applyStyle(nullptr));
}

TEST(RadioStyle, MovesCaptionIntoCentreLabel)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    radio.setText(QStringLiteral("Mode A"));

    CarbonUIQRadioButton::applyStyle(&radio);

    EXPECT_TRUE(radio.text().isEmpty()) << "caption must be moved out of the button";
    QLabel* label = centreLabel(&radio);
    ASSERT_NE(label, nullptr) << "child label 'rb_label' is missing";
    EXPECT_EQ(label->text(), QStringLiteral("Mode A"));
    EXPECT_EQ(label->alignment(), Qt::AlignCenter);
    EXPECT_TRUE(label->testAttribute(Qt::WA_TransparentForMouseEvents));
}

TEST(RadioStyle, AppliesDefaultMinimumWidthAndButtonMetrics)
{
    QRadioButton radio;

    CarbonUIQRadioButton::applyStyle(&radio);

    EXPECT_QSS_CONTAINS(radio.styleSheet(), "min-width: 28px; min-height: 24px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "QRadioButton::indicator { width: 0px; height: 0px;");
    EXPECT_EQ(radio.cursor().shape(), Qt::PointingHandCursor);
    EXPECT_EQ(radio.sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    EXPECT_EQ(radio.contentsMargins(), QMargins(0, 0, 0, 0));
}

TEST(RadioStyle, HonoursCustomMinimumWidth)
{
    QRadioButton radio;

    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_FourRound, 72);

    EXPECT_QSS_CONTAINS(radio.styleSheet(), "min-width: 72px;");
}

TEST(RadioStyle, FourRoundShapeKeepsUniformRadius)
{
    QRadioButton radio;

    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_FourRound);

    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-radius: 12px");
    EXPECT_QSS_NOT_CONTAINS(radio.styleSheet(), "border-top-left-radius");
}

TEST(RadioStyle, RectangleShapeZeroesAllCorners)
{
    QRadioButton radio;

    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_Rectangle);

    EXPECT_QSS_NOT_CONTAINS(radio.styleSheet(), "border-radius: 12px");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-top-left-radius: 0px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-bottom-right-radius: 0px;");
}

TEST(RadioStyle, LeftRoundShapeRoundsLeftEdgeOnly)
{
    QRadioButton radio;

    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_LeftRound);

    EXPECT_QSS_CONTAINS(radio.styleSheet(),
                        "border-top-left-radius: 10px; border-bottom-left-radius: 10px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-top-right-radius: 0px;");
}

TEST(RadioStyle, RightRoundShapeRoundsRightEdgeOnly)
{
    QRadioButton radio;

    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_RightRound);

    EXPECT_QSS_CONTAINS(radio.styleSheet(),
                        "border-top-right-radius: 10px; border-bottom-right-radius: 10px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-top-left-radius: 0px;");
}

TEST(RadioStyle, IsIdempotentAcrossRepeatedCalls)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    radio.setText(QStringLiteral("Once"));

    CarbonUIQRadioButton::applyStyle(&radio);
    CarbonUIQRadioButton::applyStyle(&radio);
    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_Rectangle, 40);

    EXPECT_EQ(radio.findChildren<QLabel*>(QStringLiteral("rb_label")).size(), 1)
        << "styling twice must not spawn a second caption label";
    EXPECT_EQ(centreLabel(&radio)->text(), QStringLiteral("Once"));
    EXPECT_TRUE(radio.property("_carbonui_radio_sync_installed").toBool());
}

TEST(RadioStyle, CheckedStateRecoloursCaption)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    CarbonUIQRadioButton::applyStyle(&radio);
    QLabel* label = centreLabel(&radio);
    ASSERT_NE(label, nullptr);

    EXPECT_QSS_CONTAINS(label->styleSheet(), "#cdcdcd");

    radio.setChecked(true);

    EXPECT_QSS_CONTAINS(label->styleSheet(), "#ffffff");
}

TEST(RadioStyle, CaptionChangeIsSyncedOnShowEvent)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    radio.setText(QStringLiteral("Old"));
    CarbonUIQRadioButton::applyStyle(&radio);
    QLabel* label = centreLabel(&radio);
    ASSERT_NE(label, nullptr);

    radio.setText(QStringLiteral("New"));   // documented way to change the caption
    QEvent show(QEvent::Show);
    QApplication::sendEvent(&radio, &show);

    EXPECT_EQ(label->text(), QStringLiteral("New"));
    EXPECT_TRUE(radio.text().isEmpty());
}

TEST(RadioStyle, LongCaptionStaysInsideButtonBounds)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    CarbonUIQRadioButton::applyStyle(&radio, CarbonUIQRadioButton::Radio_FourRound, 120);
    QLabel* label = centreLabel(&radio);
    ASSERT_NE(label, nullptr);

    radio.resize(120, 28);
    radio.show();
    QApplication::processEvents();          // 激活布局并让 Resize/Show 同步生效

    // 模拟语言切换：布局稳定后再改长文案（旧实现在这一步让标签被
    // 文本宽度撑出按钮边界），中英文各来一轮。
    radio.setText(QStringLiteral("这是一段非常长的中文测试文本，用于验证文字不会越出按钮边界"));
    QApplication::processEvents();
    radio.setText(QStringLiteral("and a very long English tail used to overflow both scripts"));
    QApplication::processEvents();

    EXPECT_GE(label->geometry().left(), radio.rect().left())
        << "caption label must not extend past the button's left edge";
    EXPECT_LE(label->geometry().right(), radio.rect().right())
        << "caption label must not extend past the button's right edge";
    EXPECT_GE(label->geometry().top(), radio.rect().top());
    EXPECT_LE(label->geometry().bottom(), radio.rect().bottom());
}

TEST(RadioStyle, LongCaptionDoesNotInflateButtonInLayout)
{
    // 复刻宿主页面结构：HBox(按钮 + 弹性 spacer)，页面常放进 QScrollArea
    QWidget window;
    auto* row = new QHBoxLayout(&window);
    auto* radio = new QRadioButton(&window);
    radio->setObjectName(QStringLiteral("rb"));
    row->addWidget(radio);
    row->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    CarbonUIQRadioButton::applyStyle(radio, CarbonUIQRadioButton::Radio_FourRound, 120);
    QLabel* label = centreLabel(radio);
    ASSERT_NE(label, nullptr);

    window.resize(400, 60);
    window.show();
    QApplication::processEvents();
    const int widthBefore = radio->width();

    // 模拟语言切换：布局稳定后连续改长文案；布局失效需要数轮事件循环
    // 才能收敛（setText -> 子标签 updateGeometry -> 父布局 LayoutRequest），
    // 因此每次改文案后泵多轮事件。
    for (int i = 0; i < 5; ++i)
    {
        QApplication::processEvents();
    }
    radio->setText(QStringLiteral("这是一段非常长的中文测试文本，用于验证文字不会越出按钮边界"));
    for (int i = 0; i < 5; ++i)
    {
        QApplication::processEvents();
    }
    radio->setText(QStringLiteral("and a very long English tail used to overflow both scripts"));
    for (int i = 0; i < 5; ++i)
    {
        QApplication::processEvents();
    }

    // 机制契约：文字容器必须以 Ignored 尺寸策略铺满按钮内容区（几何完全由
    // 布局给定，不随文本反推）。旧实现标签是默认 Preferred 策略，长文本会在
    // 布局失效链的中间态把按钮/标签撑到数百像素宽，绘制恰好落在该状态时
    // 文字就画到了按钮之外。
    EXPECT_EQ(label->sizePolicy().horizontalPolicy(), QSizePolicy::Ignored)
        << "caption label must fill the button (ignored policy), not follow text width";
    EXPECT_EQ(label->sizePolicy().verticalPolicy(), QSizePolicy::Ignored);
    // 按钮宽度不得被文案撑大（旧实现：布局最小宽度跟随标签文本宽度，按钮涨到数百像素，
    // 把所在行撑出滚动区视口）
    EXPECT_LE(radio->width(), qMax(widthBefore, 130))
        << "button width must not grow with caption length";
    // 标签几何必须落在按钮矩形内
    EXPECT_GE(label->geometry().left(), radio->rect().left());
    EXPECT_LE(label->geometry().right(), radio->rect().right());
    // 窗口整体最小宽度不得随文案增长
    EXPECT_LE(window.minimumSizeHint().width(), 400)
        << "window minimum width must not follow caption length";
}
