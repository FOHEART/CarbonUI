/**
 * @file  carbonui_radio_test.cpp
 * @brief `CarbonUIClass::applyRadioStyle()` 的行为用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
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
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>

#include "CarbonUIClass.h"
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
    EXPECT_NO_FATAL_FAILURE(CarbonUIClass::applyRadioStyle(nullptr));
}

TEST(RadioStyle, MovesCaptionIntoCentreLabel)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    radio.setText(QStringLiteral("Mode A"));

    CarbonUIClass::applyRadioStyle(&radio);

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

    CarbonUIClass::applyRadioStyle(&radio);

    EXPECT_QSS_CONTAINS(radio.styleSheet(), "min-width: 28px; min-height: 24px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "QRadioButton::indicator { width: 0px; height: 0px;");
    EXPECT_EQ(radio.cursor().shape(), Qt::PointingHandCursor);
    EXPECT_EQ(radio.sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    EXPECT_EQ(radio.contentsMargins(), QMargins(0, 0, 0, 0));
}

TEST(RadioStyle, HonoursCustomMinimumWidth)
{
    QRadioButton radio;

    CarbonUIClass::applyRadioStyle(&radio, CarbonUIClass::Radio_FourRound, 72);

    EXPECT_QSS_CONTAINS(radio.styleSheet(), "min-width: 72px;");
}

TEST(RadioStyle, FourRoundShapeKeepsUniformRadius)
{
    QRadioButton radio;

    CarbonUIClass::applyRadioStyle(&radio, CarbonUIClass::Radio_FourRound);

    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-radius: 12px");
    EXPECT_QSS_NOT_CONTAINS(radio.styleSheet(), "border-top-left-radius");
}

TEST(RadioStyle, RectangleShapeZeroesAllCorners)
{
    QRadioButton radio;

    CarbonUIClass::applyRadioStyle(&radio, CarbonUIClass::Radio_Rectangle);

    EXPECT_QSS_NOT_CONTAINS(radio.styleSheet(), "border-radius: 12px");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-top-left-radius: 0px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-bottom-right-radius: 0px;");
}

TEST(RadioStyle, LeftRoundShapeRoundsLeftEdgeOnly)
{
    QRadioButton radio;

    CarbonUIClass::applyRadioStyle(&radio, CarbonUIClass::Radio_LeftRound);

    EXPECT_QSS_CONTAINS(radio.styleSheet(),
                        "border-top-left-radius: 10px; border-bottom-left-radius: 10px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-top-right-radius: 0px;");
}

TEST(RadioStyle, RightRoundShapeRoundsRightEdgeOnly)
{
    QRadioButton radio;

    CarbonUIClass::applyRadioStyle(&radio, CarbonUIClass::Radio_RightRound);

    EXPECT_QSS_CONTAINS(radio.styleSheet(),
                        "border-top-right-radius: 10px; border-bottom-right-radius: 10px;");
    EXPECT_QSS_CONTAINS(radio.styleSheet(), "border-top-left-radius: 0px;");
}

TEST(RadioStyle, IsIdempotentAcrossRepeatedCalls)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    radio.setText(QStringLiteral("Once"));

    CarbonUIClass::applyRadioStyle(&radio);
    CarbonUIClass::applyRadioStyle(&radio);
    CarbonUIClass::applyRadioStyle(&radio, CarbonUIClass::Radio_Rectangle, 40);

    EXPECT_EQ(radio.findChildren<QLabel*>(QStringLiteral("rb_label")).size(), 1)
        << "styling twice must not spawn a second caption label";
    EXPECT_EQ(centreLabel(&radio)->text(), QStringLiteral("Once"));
    EXPECT_TRUE(radio.property("_carbonui_radio_sync_installed").toBool());
}

TEST(RadioStyle, CheckedStateRecoloursCaption)
{
    QRadioButton radio;
    radio.setObjectName(QStringLiteral("rb"));
    CarbonUIClass::applyRadioStyle(&radio);
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
    CarbonUIClass::applyRadioStyle(&radio);
    QLabel* label = centreLabel(&radio);
    ASSERT_NE(label, nullptr);

    radio.setText(QStringLiteral("New"));   // documented way to change the caption
    QEvent show(QEvent::Show);
    QApplication::sendEvent(&radio, &show);

    EXPECT_EQ(label->text(), QStringLiteral("New"));
    EXPECT_TRUE(radio.text().isEmpty());
}
