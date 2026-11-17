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
 * 滚动区本体只负责"无边框 + 内容底 #3b3b3b"；滚动条外观统一委托给
 * CarbonUIQScrollBar::applyStyle()（2px 细条、悬停加厚到 4px，垂直与水平
 * 同一套规则），保证滚动条样式全工程只有一个来源。
 *
 * 注意：滚动条样式必须刷在 QScrollBar 对象自己的样式表上，而不是写在
 * QScrollArea 的样式表里——控件自身样式表的优先级高于祖先样式表，且
 * QScrollArea 的 "QScrollArea QWidget { ... }" 通配规则也会命中滚动条，
 * 委托写法可以让滚动条规则稳定压过通配规则。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQScrollArea.h"
#include "CarbonUIQScrollBar.h"

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

	area->setStyleSheet("QScrollArea { border: none; }"
	                    "QScrollArea QWidget { background-color: #3b3b3b; }");

	// 垂直 / 水平滚动条统一走 CarbonUIQScrollBar：2px 细条，悬停加厚到 4px
	CarbonUIQScrollBar::applyStyle(area->verticalScrollBar());
	CarbonUIQScrollBar::applyStyle(area->horizontalScrollBar());
}
