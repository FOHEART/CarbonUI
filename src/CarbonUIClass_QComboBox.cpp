/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QComboBox.cpp
 * @brief CarbonUIClass 中下拉框样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::setupComboBoxStyle：圆角输入框外观 + 资源图片
 * 下拉箭头（图片来源 `:/img/Resources/img/arrow_down.png`，依赖
 * `Resource.qrc` 的 `/img` 前缀）。公开接口的完整参数说明写在
 * CarbonUIClass.h；本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"


void CarbonUIClass::setupComboBoxStyle(QComboBox* cb)
{
	if (!cb) return;
    QString style = (
		"QComboBox { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; border: 2px solid #5f5f5f; border-radius: 12px; padding: 2px 34px 2px 8px; } "
		"QComboBox:hover { color: #dddddd; border-color: #777777; } "
		"QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; background-color: #2f2f2f; border-left: 1px solid #5f5f5f; border-top-right-radius: 10px; border-bottom-right-radius: 10px; } "
		"QComboBox::down-arrow { width: 24px; height: 24px; image: url(:/img/Resources/img/arrow_down.png); subcontrol-origin: padding; subcontrol-position: center ; } "
		"QComboBox::down-arrow:on { image: url(:/img/Resources/img/arrow_down.png); } "
		"QComboBox::icon { image: none; } "
		"QComboBox QAbstractItemView { color: white; background-color: #2f2f2f; selection-background-color: rgb(60,140,255); selection-color: white; }"
	);
    cb->setStyleSheet(style);
}
