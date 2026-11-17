/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQComboBox.cpp
 * @brief CarbonUIQComboBox 的外观实现（外观契约见 CarbonUIQComboBox.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 圆角输入框外观 + 资源图片下拉箭头。箭头图片
 * `:/img/Resources/img/arrow_down.png` 依赖 `carbonui_rcc.qrc` 的 `/img`
 * 前缀；静态库中 qrc 注册符号由 CarbonUIClass.cpp 的构造期引用保证。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQComboBox.h"

/**
 * @brief 构造下拉框并完成外观装配（详见 CarbonUIQComboBox.h）。
 */
CarbonUIQComboBox::CarbonUIQComboBox(QWidget* parent) : QComboBox(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有下拉框应用 CarbonUI 外观（详见 CarbonUIQComboBox.h）。
 */
void CarbonUIQComboBox::applyStyle(QComboBox* cb) {
	if (cb == nullptr) {
		return;
	}

	QString style =
	    "QComboBox { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; border: 2px "
	    "solid #5f5f5f;"
	    " border-radius: 12px; padding: 2px 34px 2px 8px; } "
	    "QComboBox:hover { color: #dddddd; border-color: #777777; } "
	    "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: "
	    "28px;"
	    " background-color: #2f2f2f; border-left: 1px solid #5f5f5f; border-top-right-radius: 10px;"
	    " border-bottom-right-radius: 10px; } "
	    "QComboBox::down-arrow { width: 24px; height: 24px; image: "
	    "url(:/img/Resources/img/arrow_down.png);"
	    " subcontrol-origin: padding; subcontrol-position: center ; } "
	    "QComboBox::down-arrow:on { image: url(:/img/Resources/img/arrow_down.png); } "
	    "QComboBox::icon { image: none; } "
	    "QComboBox QAbstractItemView { color: white; background-color: #2f2f2f;"
	    " selection-background-color: rgb(60,140,255); selection-color: white; }";
	cb->setStyleSheet(style);
}
