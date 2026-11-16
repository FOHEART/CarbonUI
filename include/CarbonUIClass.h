/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass.h
 * @brief 全局样式助手：QToolTip 全局配色与递归标签配色。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 本类只保留作用于全局或不绑定单一控件类的助手；单控件外观一律由自带外观的
 * CarbonUIQ* 控件类负责（CarbonUIQPushButton 等，按 CarbonUIDialog 范式实现，
 * 构造即完成装配，另提供静态 `applyStyle()` 供外部已有控件复用）。
 *
 * 线程约定：两个接口都会触碰 QWidget / QApplication，只允许在 GUI 主线程调用。
 * 错误处理：传入 NULL 时静默返回，调用方无需判空。
 */

#ifndef CARBONUICLASS_H
#define CARBONUICLASS_H

#include "carbonui_global.h"

#include <QtWidgets/QWidget>

/**
 * @brief 全局样式助手集合（全静态、无数据成员）。
 *
 * @details
 * 两个接口互不依赖：
 * - @ref applyTooltipStyle 修改 `qApp` 的样式表，影响全部控件的提示框；
 * - @ref setWidgetLabelsStyleRecursively 遍历调用方给定的子树改写标签颜色。
 *
 * 线程安全：非线程安全，只能在 GUI 主线程使用。
 */
class CARBONUI_DLL_API CarbonUIClass {
  public:
	/**
	 * @brief 给整个应用补一条 `QToolTip` 样式（深底、蓝字、2px 边框）。
	 *
	 * @details
	 * 实现是"追加"而不是"替换"：把 `QToolTip` 规则拼接到
	 * `qApp->styleSheet()` 末尾，因此重复调用会重复追加同样的规则。
	 * `qApp` 为 NULL（尚无 QApplication）时静默返回。
	 *
	 * @note 这是本类唯一修改全局状态的接口，会作用于所有控件。
	 */
	static void applyTooltipStyle();

	/**
	 * @brief 递归地把所有后代 QLabel 的文字颜色改成次级色 \#676767。
	 *
	 * @details
	 * 只处理 QLabel，其它控件（含 QPushButton 等）的样式表保持原样；
	 * 已有样式表会被整体覆盖为 `color: \#676767;`。
	 *
	 * @param[in,out] w 遍历起点，**可为 NULL**，为 NULL 时直接返回。
	 */
	static void setWidgetLabelsStyleRecursively(QWidget* w);
};

#endif // CARBONUICLASS_H
