/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQStatusBar.h
 * @brief 状态栏控件：构造即获得 CarbonUI 暗色状态栏外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS
 * 装配，调用方只需 new 后交给 QMainWindow::setStatusBar()。
 *
 * 外观契约：深色底 \#2e2e2e + 次级文字色 \#676767、无边框无轮廓。
 *
 * 需要给"外部已有的 QStatusBar"（例如 Qt Designer 的 ui->statusBar）刷同样
 * 外观的调用方，可改用静态接口 CarbonUIQStatusBar::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQSTATUSBAR_H
#define CARBONUIQSTATUSBAR_H

#include "carbonui_global.h"

#include <QtWidgets/QStatusBar>

/**
 * @brief CarbonUI 状态栏：自带暗色 QSS 的 QStatusBar 子类。
 */
class CARBONUI_DLL_API CarbonUIQStatusBar : public QStatusBar {
  public:
	/**
	 * @brief 构造状态栏并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQStatusBar(QWidget* parent = nullptr);

	/**
	 * @brief 给已有状态栏应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] sb 目标状态栏，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QStatusBar* sb);
};

#endif // CARBONUIQSTATUSBAR_H
