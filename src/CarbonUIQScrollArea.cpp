/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQScrollArea.cpp
 * @brief CarbonUIQScrollArea 的外观实现（外观契约见 CarbonUIQScrollArea.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 无边框、内容底 \#3b3b3b，纵向滚动条收窄到 4px（\#cdcdcd 滑块、无加减页与
 * 上下箭头按钮）。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQScrollArea.h"
#include <QtWidgets/QScrollBar>

/**
 * @brief 构造滚动区并完成外观装配（详见 CarbonUIQScrollArea.h）。
 */
CarbonUIQScrollArea::CarbonUIQScrollArea(QWidget* parent) : QScrollArea(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有滚动区应用 CarbonUI 外观（详见 CarbonUIQScrollArea.h）。
 */
void CarbonUIQScrollArea::applyStyle(QScrollArea* area) {
	if (area == nullptr) {
		return;
	}

	area->setStyleSheet(
	    "QScrollArea { border: none; }"
	    "QScrollArea QWidget { background-color: #3b3b3b; }"
	    "QScrollArea QScrollBar:vertical { width: 4px; background: transparent; margin: 0px; }"
	    "QScrollArea QScrollBar::handle:vertical { background: #cdcdcd; min-height: 20px; "
	    "border-radius: 4px; }"
	    "QScrollArea QScrollBar::add-line:vertical, QScrollArea QScrollBar::sub-line:vertical { "
	    "height: 0px; }"
	    "QScrollArea QScrollBar::add-page:vertical, QScrollArea QScrollBar::sub-page:vertical { "
	    "background: none; }");

	// ensure scrollbars have minimal width
	QScrollBar* verticalBar = area->verticalScrollBar();
	if (verticalBar != nullptr) {
		verticalBar->setMinimumWidth(4);
	}
}
