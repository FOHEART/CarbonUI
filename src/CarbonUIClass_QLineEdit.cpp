/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QLineEdit.cpp
 * @brief CarbonUIClass 中单行输入框样式助手及其内部辅助件的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 数字输入框样式（居中文本、聚焦全选、C 语言环境数值校验）的实现文件。
 * 聚焦全选依赖内部辅助类 SelectAllOnFocusFilter：`QLineEdit` 自身没有
 * "获得焦点即全选"的开关，用 `QTimer::singleShot(0, ...)` 把全选推迟到
 * Qt 完成自己的焦点/鼠标处理之后执行。
 *
 * 公开接口 @ref CarbonUIClass::applyLineEditStyle_numberInput 的完整参数说明
 * 写在 CarbonUIClass.h；本文件只补充实现细节与内部辅助件的约定。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtCore/QEvent>
#include <QtCore/QLocale>
#include <QtCore/QTimer>
#include <QtGui/QDoubleValidator>

/**
 * @brief 聚焦时自动全选文本的事件过滤器。
 *
 * @details
 * 数字输入框的典型用法是"整段替换"：鼠标点进来、或键盘 Tab 进来时把原有数值
 * 全选，用户直接敲新值即可覆盖。因为 `QLineEdit` 自身没有这个开关，这里用
 * `QTimer::singleShot(0, ...)` 把全选推迟到 Qt 完成自己的焦点/鼠标处理后执行，
 * 否则会在鼠标按下时被随后的光标定位覆盖。
 *
 * 该过滤器以被监视的 `QLineEdit` 为父对象，随输入框一起销毁。
 */
class SelectAllOnFocusFilter : public QObject
{
public:
    /**
     * @brief 构造过滤器。
     * @param[in] parent 父对象，通常传被监视的 QLineEdit；可为 NULL。
     */
    SelectAllOnFocusFilter(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief 在获得焦点或用鼠标按下时把文本全选。
     *
     * @param[in] watched 事件接收者，期望是 QLineEdit；其它类型原样放行。
     * @param[in] event   事件对象，不可为 NULL。
     * @return 恒定返回 false，事件继续交给 QLineEdit 自身处理。
     */
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (auto le = qobject_cast<QLineEdit*>(watched))
        {
            if (event->type() == QEvent::FocusIn)
            {
                QTimer::singleShot(0, le, [le]() { le->selectAll(); });
            }
            else if (event->type() == QEvent::MouseButtonPress)
            {
                if (!le->hasFocus())
                {
                    QTimer::singleShot(0, le, [le]() { le->selectAll(); });
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};


void CarbonUIClass::applyLineEditStyle_numberInput(QLineEdit* le, int width)
{
    if (!le) return;

    le->setStyleSheet(
        "QLineEdit { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; border: 2px solid #5f5f5f; border-radius: 12px; padding: 2px 5px; } "
        "QLineEdit:hover { color: #dddddd; border-color: #777777; } ");

    le->setCursor(Qt::IBeamCursor);
    le->setContentsMargins(0, 0, 0, 0);
    le->setAlignment(Qt::AlignCenter);

    if (width > 0)
        le->setFixedWidth(width);

    auto* filter = new SelectAllOnFocusFilter(le);
    le->installEventFilter(filter);

    QObject::connect(le, &QLineEdit::returnPressed, le, [le]() { le->clearFocus(); });

    auto* validator = new QDoubleValidator(-1e12, 1e12, 6, le);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    le->setValidator(validator);
}
