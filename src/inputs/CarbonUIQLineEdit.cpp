/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQLineEdit.cpp
 * @brief CarbonUIQLineEdit 的外观与内部辅助件实现（契约见 CarbonUIQLineEdit.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 数字输入框样式（居中文本、聚焦全选、C 语言环境数值校验）的实现文件。
 * 聚焦全选依赖内部辅助类 SelectAllOnFocusFilter：@c QLineEdit 自身没有
 * "获得焦点即全选"的开关，用 @c QTimer::singleShot(0, ...) 把全选推迟到
 * Qt 完成自己的焦点/鼠标处理之后执行。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQLineEdit.h"
#include <QtCore/QEvent>
#include <QtCore/QLocale>
#include <QtCore/QTimer>
#include <QtGui/QDoubleValidator>

namespace {
/**
 * @brief 聚焦时自动全选文本的事件过滤器。
 *
 * @details
 * 数字输入框的典型用法是"整段替换"：鼠标点进来、或键盘 Tab 进来时把原有数值
 * 全选，用户直接敲新值即可覆盖。因为 @c QLineEdit 自身没有这个开关，这里用
 * @c QTimer::singleShot(0, ...) 把全选推迟到 Qt 完成自己的焦点/鼠标处理后执行，
 * 否则会在鼠标按下时被随后的光标定位覆盖。
 *
 * 该过滤器以被监视的 @c QLineEdit 为父对象，随输入框一起销毁。
 */
class SelectAllOnFocusFilter : public QObject {
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
	bool eventFilter(QObject* watched, QEvent* event) override {
		auto* le = qobject_cast<QLineEdit*>(watched);
		if (le != nullptr) {
			if (event->type() == QEvent::FocusIn) {
				QTimer::singleShot(0, le, [le] { le->selectAll(); });
			} else if (event->type() == QEvent::MouseButtonPress && !le->hasFocus()) {
				QTimer::singleShot(0, le, [le] { le->selectAll(); });
			}
		}
		return QObject::eventFilter(watched, event);
	}
};
} // namespace

/**
 * @brief 构造数字输入框并完成装配（详见 CarbonUIQLineEdit.h）。
 */
CarbonUIQLineEdit::CarbonUIQLineEdit(QWidget* parent, int width)
    : QLineEdit(parent), m_fixedWidth(width) {
	applyStyle(this, m_fixedWidth);
}

/**
 * @brief 查询固定宽度（详见 CarbonUIQLineEdit.h）。
 */
int CarbonUIQLineEdit::fixedWidth() const {
	return m_fixedWidth;
}

/**
 * @brief 给已有输入框应用 CarbonUI 数字输入框外观（详见 CarbonUIQLineEdit.h）。
 */
void CarbonUIQLineEdit::applyStyle(QLineEdit* le, int width) {
	if (le == nullptr) {
		return;
	}

	le->setStyleSheet("QLineEdit { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; "
	                  "border: 2px solid #5f5f5f;"
	                  " border-radius: 12px; padding: 2px 5px; } "
	                  "QLineEdit:hover { color: #dddddd; border-color: #777777; } ");

	le->setCursor(Qt::IBeamCursor);
	le->setContentsMargins(0, 0, 0, 0);
	le->setAlignment(Qt::AlignCenter);

	if (width > 0) {
		le->setFixedWidth(width);
	}

	auto* filter = new SelectAllOnFocusFilter(le);
	le->installEventFilter(filter);

	QObject::connect(le, &QLineEdit::returnPressed, le, [le] { le->clearFocus(); });

	auto* validator = new QDoubleValidator(-1e12, 1e12, 6, le);
	validator->setNotation(QDoubleValidator::StandardNotation);
	validator->setLocale(QLocale::c());
	le->setValidator(validator);
}
