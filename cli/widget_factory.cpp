/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  widget_factory.cpp
 * @brief 控件工厂实现：每种控件一个分支，尺寸取"恰好容得下默认文案"的值。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#include "widget_factory.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

#include "CarbonUIQCheckBox.h"
#include "CarbonUIQComboBox.h"
#include "CarbonUIQLineEdit.h"
#include "CarbonUIQPushButton.h"
#include "CarbonUIQRadioButton.h"
#include "CarbonUIQStatusBar.h"
#include "CarbonUIQTabWidget.h"
#include "CarbonUIQTreeWidget.h"

#include "cli_parser.h"
#include "groupboxcollapser.h"

namespace carbonui {
namespace cli {

namespace {

/// @brief 取 `spec.text`，为空时回退到该类型的默认文案。
QString textOr(const WidgetSpec& spec, const QString& fallback)
{
    return spec.text.isEmpty() ? fallback : spec.text;
}

/// @brief button / radio 的外形由字符串映射而来，非法时写 @p error 并返回 false。
bool buildButton(QWidget** out, const WidgetSpec& spec, QString* error)
{
    CarbonUIQPushButton::ButtonShape shape = CarbonUIQPushButton::Shape_RoundAll;
    if (!parseButtonShape(spec.shape, &shape))
    {
        *error = QStringLiteral("未知的 --shape 取值 %1（允许 round|left|right|rect）").arg(spec.shape);
        return false;
    }
    auto* btn = new QPushButton(textOr(spec, QStringLiteral("Confirm")));
    CarbonUIQPushButton::applyStyle(btn, shape);
    if (spec.state == QLatin1String("disabled"))
        btn->setEnabled(false);
    btn->resize(120, 36);
    *out = btn;
    return true;
}

bool buildRadio(QWidget** out, const WidgetSpec& spec, QString* error)
{
    CarbonUIQRadioButton::RadioShape shape = CarbonUIQRadioButton::Radio_FourRound;
    if (!parseRadioShape(spec.shape, &shape))
    {
        *error = QStringLiteral("未知的 --shape 取值 %1（允许 round|left|right|rect）").arg(spec.shape);
        return false;
    }
    auto* rb = new QRadioButton(textOr(spec, QStringLiteral("Mode")));
    CarbonUIQRadioButton::applyStyle(rb, shape);
    if (spec.state == QLatin1String("checked"))
        rb->setChecked(true);
    if (spec.state == QLatin1String("disabled"))
        rb->setEnabled(false);
    rb->resize(90, 30);
    *out = rb;
    return true;
}

bool buildCheckbox(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(error);
    auto* cb = new QCheckBox();
    CarbonUIQCheckBox::applyStyle(cb);
    if (spec.state != QLatin1String("normal"))
        cb->setChecked(spec.state == QLatin1String("checked"));
    if (spec.state == QLatin1String("disabled"))
        cb->setEnabled(false);
    cb->resize(16, 16);
    *out = cb;
    return true;
}

bool buildLineEdit(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(error);
    auto* le = new QLineEdit(textOr(spec, QStringLiteral("42.5")));
    CarbonUIQLineEdit::applyStyle(le);
    if (spec.state == QLatin1String("disabled"))
        le->setEnabled(false);
    le->resize(120, 28);
    *out = le;
    return true;
}

bool buildTab(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(spec);
    Q_UNUSED(error);
    auto* tw = new QTabWidget();
    CarbonUIQTabWidget::applyStyle(tw);
    auto* first = new QWidget();
    auto* layout = new QVBoxLayout(first);
    layout->addWidget(new QLabel(QStringLiteral("first page")));
    layout->setContentsMargins(8, 8, 8, 8);
    tw->addTab(first, QStringLiteral("Tab one"));
    tw->addTab(new QWidget(), QStringLiteral("Tab two"));
    tw->resize(220, 140);
    *out = tw;
    return true;
}

bool buildCombo(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(error);
    auto* combo = new QComboBox();
    CarbonUIQComboBox::applyStyle(combo);
    combo->addItem(textOr(spec, QStringLiteral("Item one")));
    combo->addItem(QStringLiteral("Item two"));
    if (spec.state == QLatin1String("disabled"))
        combo->setEnabled(false);
    combo->resize(160, 28);
    *out = combo;
    return true;
}

bool buildTree(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(spec);
    Q_UNUSED(error);
    auto* tree = new QTreeWidget();
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList() << QStringLiteral("Name") << QStringLiteral("Value"));
    auto* root = new QTreeWidgetItem(tree, QStringList() << QStringLiteral("Group") << QString());
    new QTreeWidgetItem(root, QStringList() << QStringLiteral("width") << QStringLiteral("320"));
    new QTreeWidgetItem(root, QStringList() << QStringLiteral("height") << QStringLiteral("240"));
    tree->expandAll();
    CarbonUIQTreeWidget::applyStyle(tree);
    tree->resize(260, 150);
    *out = tree;
    return true;
}

bool buildStatusBar(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(error);
    auto* bar = new QStatusBar();
    CarbonUIQStatusBar::applyStyle(bar);
    bar->showMessage(textOr(spec, QStringLiteral("Ready")));
    bar->resize(320, 24);
    *out = bar;
    return true;
}

bool buildGroupBox(QWidget** out, const WidgetSpec& spec, QString* error)
{
    Q_UNUSED(error);
    // 折叠控制器要求"已设置布局"的分组框；内容给一个标签 + 一个样式按钮，
    // 与视觉回归基线 groupbox-expanded.png 的构图保持一致。
    auto* box = new QGroupBox(textOr(spec, QStringLiteral("Advanced")));
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(new QLabel(QStringLiteral("body text")));
    auto* action = new QPushButton(QStringLiteral("Action"));
    CarbonUIQPushButton::applyStyle(action);
    layout->addWidget(action);
    const bool expanded = spec.state != QLatin1String("collapsed");
    QGroupBoxCollapser::installIntoGroupBox(box, nullptr, expanded);
    box->resize(240, box->sizeHint().height());
    *out = box;
    return true;
}

} // namespace

QWidget* createStyledWidget(const WidgetSpec& spec, QString* error)
{
    if (error)
        error->clear();

    QWidget* widget = nullptr;
    bool ok = false;
    if (spec.type == QLatin1String("button"))
        ok = buildButton(&widget, spec, error);
    else if (spec.type == QLatin1String("radio"))
        ok = buildRadio(&widget, spec, error);
    else if (spec.type == QLatin1String("checkbox"))
        ok = buildCheckbox(&widget, spec, error);
    else if (spec.type == QLatin1String("lineedit"))
        ok = buildLineEdit(&widget, spec, error);
    else if (spec.type == QLatin1String("tab"))
        ok = buildTab(&widget, spec, error);
    else if (spec.type == QLatin1String("combo"))
        ok = buildCombo(&widget, spec, error);
    else if (spec.type == QLatin1String("tree"))
        ok = buildTree(&widget, spec, error);
    else if (spec.type == QLatin1String("statusbar"))
        ok = buildStatusBar(&widget, spec, error);
    else if (spec.type == QLatin1String("groupbox"))
        ok = buildGroupBox(&widget, spec, error);
    else if (error)
        *error = QStringLiteral("未知的 --widget 取值 %1（允许 %2）")
                     .arg(spec.type, widgetTypes().join(QStringLiteral("|")));

    if (!ok)
        return nullptr;
    return widget;
}

} // namespace cli
} // namespace carbonui
