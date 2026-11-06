/**
 * @file  carbonui_button_test.cpp
 * @brief `CarbonUIClass::applyButtonStyle()` 的圆角形状用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 四个 `ButtonShape` 取值映射到四组不同的圆角半径，是拼接按钮（左圆+右圆）
 * 的唯一手段，因此逐形状断言完整半径串；同时断言默认参数等于 Shape_RoundAll，
 * 防止默认值被改掉时调用方外观静默变化。
 */

#include <gtest/gtest.h>

#include <QtWidgets/QPushButton>

#include "CarbonUIClass.h"
#include "qt_test_support.h"

namespace
{
::testing::AssertionResult cornersMatch(const QPushButton& button, int topLeft,
                                        int bottomLeft, int topRight, int bottomRight)
{
    const QString expected =
        QStringLiteral("border-top-left-radius: %1px; border-bottom-left-radius: %2px; "
                       "border-top-right-radius: %3px; border-bottom-right-radius: %4px;")
            .arg(topLeft).arg(bottomLeft).arg(topRight).arg(bottomRight);
    return carbonui_test::qssContains(button.styleSheet(), expected.toUtf8().constData());
}
} // namespace

TEST(ButtonStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIClass::applyButtonStyle(nullptr));
}

TEST(ButtonStyle, DefaultShapeRoundsAllCorners)
{
    QPushButton button;

    CarbonUIClass::applyButtonStyle(&button);

    EXPECT_TRUE(cornersMatch(button, 10, 10, 10, 10));
}

TEST(ButtonStyle, LeftRoundShapeRoundsLeftEdgeOnly)
{
    QPushButton button;

    CarbonUIClass::applyButtonStyle(&button, CarbonUIClass::Shape_LeftRound);

    EXPECT_TRUE(cornersMatch(button, 10, 10, 0, 0));
}

TEST(ButtonStyle, RightRoundShapeRoundsRightEdgeOnly)
{
    QPushButton button;

    CarbonUIClass::applyButtonStyle(&button, CarbonUIClass::Shape_RightRound);

    EXPECT_TRUE(cornersMatch(button, 0, 0, 10, 10));
}

TEST(ButtonStyle, RectangleShapeRoundsNothing)
{
    QPushButton button;

    CarbonUIClass::applyButtonStyle(&button, CarbonUIClass::Shape_Rectangle);

    EXPECT_TRUE(cornersMatch(button, 0, 0, 0, 0));
}

TEST(ButtonStyle, AppliesAccentPaletteAndPointerCursor)
{
    QPushButton button;

    CarbonUIClass::applyButtonStyle(&button);

    EXPECT_EQ(button.cursor().shape(), Qt::PointingHandCursor);
    EXPECT_QSS_CONTAINS(button.styleSheet(), "background-color: #009ee8");
    EXPECT_QSS_CONTAINS(button.styleSheet(), "QPushButton:hover { background-color: #00b0ff; }");
    EXPECT_QSS_CONTAINS(button.styleSheet(), "QPushButton:pressed { background-color: #007bb3; }");
    EXPECT_QSS_CONTAINS(button.styleSheet(), "QPushButton:disabled { background-color: #636363");
}

TEST(ButtonStyle, DisabledRuleSurvivesShapeVariants)
{
    // 禁用态规则与圆角形状无关：任何形状都不允许漏掉 disabled 分支，
    // 否则按钮在 setEnabled(false) 后仍渲染成可用态（截图基线曾抓到过）。
    QPushButton round;
    QPushButton rectangle;
    CarbonUIClass::applyButtonStyle(&round, CarbonUIClass::Shape_RoundAll);
    CarbonUIClass::applyButtonStyle(&rectangle, CarbonUIClass::Shape_Rectangle);

    EXPECT_QSS_CONTAINS(round.styleSheet(), "QPushButton:disabled");
    EXPECT_QSS_CONTAINS(rectangle.styleSheet(), "QPushButton:disabled");
}
