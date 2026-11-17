/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQStatusBar.cpp
 * @brief CarbonUIQStatusBar 的外观实现（外观契约见 CarbonUIQStatusBar.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 深色底 \#2e2e2e + 次级文字色 \#676767、无边框无轮廓。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQStatusBar.h"

/**
 * @brief 构造状态栏并完成外观装配（详见 CarbonUIQStatusBar.h）。
 */
CarbonUIQStatusBar::CarbonUIQStatusBar(QWidget* parent) : QStatusBar(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有状态栏应用 CarbonUI 外观（详见 CarbonUIQStatusBar.h）。
 */
void CarbonUIQStatusBar::applyStyle(QStatusBar* sb) {
	if (sb == nullptr) {
		return;
	}

	sb->setStyleSheet("background-color: #2e2e2e; color: #676767; border: none; outline: none;");
}
