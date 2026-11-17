/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQPushButton.cpp
 * @brief CarbonUIQPushButton 的外观实现（外形契约见 CarbonUIQPushButton.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 圆角组合只决定四角 10px / 0px 的分配，用于把多个按钮拼成分段选择器；
 * 悬停 / 按下 / 禁用三态配色全部写在一条 QSS 字面量里，没有主题系统，
 * 改配色就是改这些字面量。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQPushButton.h"

/**
 * @brief 构造按钮并应用外形（详见 CarbonUIQPushButton.h）。
 */
CarbonUIQPushButton::CarbonUIQPushButton(QWidget* parent, ButtonShape shape)
    : QPushButton(parent), m_shape(shape) {
	applyStyle(this, m_shape);
}

/**
 * @brief 构造带文字的按钮并应用外形（详见 CarbonUIQPushButton.h）。
 */
CarbonUIQPushButton::CarbonUIQPushButton(const QString& text, QWidget* parent, ButtonShape shape)
    : QPushButton(text, parent), m_shape(shape) {
	applyStyle(this, m_shape);
}

/**
 * @brief 切换外形并重刷样式表（详见 CarbonUIQPushButton.h）。
 */
void CarbonUIQPushButton::setButtonShape(ButtonShape shape) {
	m_shape = shape;
	applyStyle(this, m_shape);
}

/**
 * @brief 查询当前外形（详见 CarbonUIQPushButton.h）。
 */
CarbonUIQPushButton::ButtonShape CarbonUIQPushButton::buttonShape() const {
	return m_shape;
}

/**
 * @brief 给已有按钮应用 CarbonUI 外观（详见 CarbonUIQPushButton.h）。
 */
void CarbonUIQPushButton::applyStyle(QPushButton* btn, ButtonShape shape) {
	if (btn == nullptr) {
		return;
	}

	// determine corner radii depending on shape
	int tl = 0;
	int tr = 0;
	int bl = 0;
	int br = 0;
	switch (shape) {
	case Shape_RoundAll:
		tl = tr = bl = br = 10;
		break;
	case Shape_LeftRound:
		tl = bl = 10;
		tr = br = 0;
		break;
	case Shape_Rectangle:
		tl = tr = bl = br = 0;
		break;
	case Shape_RightRound:
		tr = br = 10;
		tl = bl = 0;
		break;
	default:
		tl = tr = bl = br = 0;
		break;
	}

	QString stylesheet = QString("QPushButton { background-color: #009ee8; color: #ffffff; border: "
	                             "none; padding: 0px 2px; min-height: 28px; "
	                             "border-top-left-radius: %1px; border-bottom-left-radius: %2px; "
	                             "border-top-right-radius: %3px;"
	                             " border-bottom-right-radius: %4px; }")
	                         .arg(tl)
	                         .arg(bl)
	                         .arg(tr)
	                         .arg(br);
	stylesheet += "QPushButton:hover { background-color: #00b0ff; }";
	stylesheet += "QPushButton:pressed { background-color: #007bb3; }";
	// 禁用态必须有独立规则，否则按钮被 setEnabled(false) 后仍渲染成可用态，
	// 用户分不清能不能点。灰底取自主题里既有的"非激活灰"，文字压暗但保持可读。
	stylesheet += "QPushButton:disabled { background-color: #636363; color: #cfcfcf; }";

	btn->setStyleSheet(stylesheet);
	btn->setCursor(Qt::PointingHandCursor);
}
