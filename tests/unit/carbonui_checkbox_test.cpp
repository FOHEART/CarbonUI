/**
 * @file  carbonui_checkbox_test.cpp
 * @brief `CarbonUIClass::applyCheckStyle()` 的行为用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui_tests
 *
 * @details
 * 覆盖点：空指针安全、文本转移到子控件、固定几何、指示器颜色随勾选状态变化、
 * 悬停状态切换，以及本地事件过滤器对左键释放的拦截（含非左键不得误触）。
 */

#include <gtest/gtest.h>

#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidget>

#include "CarbonUIClass.h"
#include "qt_test_support.h"

namespace
{
QWidget* findIndicator(QCheckBox* checkBox)
{
    return checkBox->findChild<QWidget*>(checkBox->objectName() + QStringLiteral("_indicator"));
}
} // namespace

TEST(CheckStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIClass::applyCheckStyle(nullptr));
}

TEST(CheckStyle, MovesTextIntoIndicatorChildWidget)
{
    QCheckBox checkBox;
    checkBox.setObjectName(QStringLiteral("chk"));
    checkBox.setText(QStringLiteral("Enable"));

    CarbonUIClass::applyCheckStyle(&checkBox);

    EXPECT_TRUE(checkBox.text().isEmpty()) << "the caption must move to the indicator subtree";
    ASSERT_NE(findIndicator(&checkBox), nullptr) << "indicator child 'chk_indicator' is missing";

    QWidget* indicator = findIndicator(&checkBox);
    EXPECT_EQ(indicator->parentWidget(), &checkBox);
    EXPECT_EQ(indicator->size(), QSize(8, 8));
    EXPECT_TRUE(indicator->testAttribute(Qt::WA_TransparentForMouseEvents));
}

TEST(CheckStyle, FixesWidgetGeometryAndCursor)
{
    QCheckBox checkBox;
    CarbonUIClass::applyCheckStyle(&checkBox);

    EXPECT_EQ(checkBox.size(), QSize(16, 16));
    EXPECT_EQ(checkBox.width(), 16);
    EXPECT_EQ(checkBox.height(), 16);
    EXPECT_EQ(checkBox.cursor().shape(), Qt::PointingHandCursor);
    EXPECT_TRUE(checkBox.testAttribute(Qt::WA_Hover));
}

TEST(CheckStyle, IndicatorColourFollowsCheckedState)
{
    QCheckBox checkBox;
    checkBox.setObjectName(QStringLiteral("chk"));
    CarbonUIClass::applyCheckStyle(&checkBox);
    QWidget* indicator = findIndicator(&checkBox);
    ASSERT_NE(indicator, nullptr);

    EXPECT_QSS_CONTAINS(indicator->styleSheet(), "#636363");

    checkBox.setChecked(true);

    EXPECT_QSS_CONTAINS(indicator->styleSheet(), "#009ee8");

    checkBox.setChecked(false);

    EXPECT_QSS_CONTAINS(indicator->styleSheet(), "#636363");
}

TEST(CheckStyle, HoverHighlightsBorderAndRestoresItOnLeave)
{
    QCheckBox checkBox;
    CarbonUIClass::applyCheckStyle(&checkBox);

    QEvent enter(QEvent::Enter);
    QApplication::sendEvent(&checkBox, &enter);
    EXPECT_QSS_CONTAINS(checkBox.styleSheet(), "#ffffff");

    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(&checkBox, &leave);
    EXPECT_QSS_CONTAINS(checkBox.styleSheet(), "#636363");
}

TEST(CheckStyle, LeftButtonReleaseTogglesCheckedState)
{
    QCheckBox checkBox;
    CarbonUIClass::applyCheckStyle(&checkBox);
    ASSERT_FALSE(checkBox.isChecked());

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(8, 8), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&checkBox, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(8, 8), Qt::LeftButton,
                        Qt::LeftButton, Qt::NoModifier);
    const bool consumed = QApplication::sendEvent(&checkBox, &release);

    EXPECT_TRUE(consumed) << "the hover filter must consume the release event";
    EXPECT_TRUE(checkBox.isChecked());
}

TEST(CheckStyle, NonLeftButtonReleaseDoesNotToggle)
{
    QCheckBox checkBox;
    CarbonUIClass::applyCheckStyle(&checkBox);

    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(8, 8), Qt::RightButton,
                        Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(&checkBox, &release);

    EXPECT_FALSE(checkBox.isChecked()) << "only the left button may toggle the checkbox";
}
