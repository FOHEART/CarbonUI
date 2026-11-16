/**
 * @file  carbonui_lineedit_test.cpp
 * @brief `CarbonUIQLineEdit::applyStyle()` 的行为用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 覆盖点：空指针安全、QSS 落地、对齐/光标、固定宽度边界、数字校验器语义、
 * 回车后失焦。所有断言都针对调用方可以观测到的控件状态，而不是 QSS 字符串
 * 的逐字相等，避免样式微调把用例拖红。
 */

#include <gtest/gtest.h>

#include <QtGui/QDoubleValidator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "CarbonUIQLineEdit.h"
#include "qt_test_support.h"

TEST(LineEditStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQLineEdit::applyStyle(nullptr));
}

TEST(LineEditStyle, AppliesNumberInputStyleSheet)
{
    QLineEdit edit;                                          // Arrange
    CarbonUIQLineEdit::applyStyle(&edit);     // Act

    EXPECT_QSS_CONTAINS(edit.styleSheet(), "QLineEdit");                    // Assert
    EXPECT_QSS_CONTAINS(edit.styleSheet(), "background-color: #2f2f2f");
    EXPECT_QSS_CONTAINS(edit.styleSheet(), "border-radius: 12px");
}

TEST(LineEditStyle, CentersTextAndShowsBeamCursor)
{
    QLineEdit edit;
    edit.setAlignment(Qt::AlignRight);

    CarbonUIQLineEdit::applyStyle(&edit);

    EXPECT_EQ(edit.alignment(), Qt::AlignCenter);
    EXPECT_EQ(edit.contentsMargins(), QMargins(0, 0, 0, 0));
    EXPECT_EQ(edit.cursor().shape(), Qt::IBeamCursor);
}

TEST(LineEditStyle, AppliesFixedWidthOnlyForPositiveWidth)
{
    QLineEdit fixedEdit;
    QLineEdit autoEdit;

    CarbonUIQLineEdit::applyStyle(&fixedEdit, 160);
    CarbonUIQLineEdit::applyStyle(&autoEdit, 0);

    EXPECT_EQ(fixedEdit.minimumWidth(), 160);
    EXPECT_EQ(fixedEdit.maximumWidth(), 160);
    EXPECT_EQ(autoEdit.minimumWidth(), 0);
    EXPECT_EQ(autoEdit.maximumWidth(), QWIDGETSIZE_MAX);
}

TEST(LineEditStyle, InstallsCLocaleStandardNotationValidator)
{
    QLineEdit edit;

    CarbonUIQLineEdit::applyStyle(&edit);

    const auto* validator = dynamic_cast<const QDoubleValidator*>(edit.validator());
    ASSERT_NE(validator, nullptr) << "a QDoubleValidator must be installed";
    EXPECT_EQ(validator->notation(), QDoubleValidator::StandardNotation);
    // C locale keeps "1.5" acceptable on machines whose system locale uses ','
    EXPECT_EQ(validator->locale(), QLocale::c());
}

TEST(LineEditStyle, ValidatorAcceptsDecimalsAndRejectsGarbage)
{
    QLineEdit edit;
    CarbonUIQLineEdit::applyStyle(&edit);
    const QValidator* validator = edit.validator();
    ASSERT_NE(validator, nullptr);

    int pos = 0;
    QString decimal = QStringLiteral("12.5");
    EXPECT_EQ(validator->validate(decimal, pos), QValidator::Acceptable);

    pos = 0;
    QString exponent = QStringLiteral("1e5");            // StandardNotation forbids 'e'
    EXPECT_EQ(validator->validate(exponent, pos), QValidator::Invalid);

    pos = 0;
    QString text = QStringLiteral("abc");
    EXPECT_EQ(validator->validate(text, pos), QValidator::Invalid);

    pos = 0;
    QString tooLarge = QStringLiteral("9999999999999");  // > 1e12 upper bound
    EXPECT_NE(validator->validate(tooLarge, pos), QValidator::Acceptable)
        << "values outside [-1e12, 1e12] must not be accepted";
}

TEST(LineEditStyle, ReturnPressedReleasesKeyboardFocus)
{
    QWidget window;
    auto* layout = new QVBoxLayout(&window);
    auto* edit = new QLineEdit(&window);
    layout->addWidget(edit);
    CarbonUIQLineEdit::applyStyle(edit);

    carbonui_test::showAndSettle(&window);
    QApplication::setActiveWindow(&window);
    edit->setFocus();
    carbonui_test::spinUntil([edit]() { return edit->hasFocus(); });
    ASSERT_TRUE(edit->hasFocus()) << "focus setup failed; the rest of the case is meaningless";

    edit->returnPressed();

    EXPECT_FALSE(edit->hasFocus());
}
