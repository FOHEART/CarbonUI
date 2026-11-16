/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQToolBar.h
 * @brief 工具栏控件：构造即获得 CarbonUI 暗色工具栏外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS
 * 与几何设置，调用方只需 new 后交给 QMainWindow::addToolBar()。
 *
 * 外观契约：禁止拖动/浮动、固定 32px 高、28px 图标，工具栏底 \#3b3b3b、
 * 顶部 1px \#212121 分隔线、按钮悬停 1px \#5d5d5d 描边。
 *
 * 需要给"外部已有的 QToolBar"（例如 Qt Designer 的 ui->mainToolBar）刷同样
 * 外观的调用方，可改用静态接口 CarbonUIQToolBar::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQTOOLBAR_H
#define CARBONUIQTOOLBAR_H

#include "carbonui_global.h"

#include <QtWidgets/QToolBar>

/**
 * @brief CarbonUI 工具栏：自带暗色 QSS 的 QToolBar 子类。
 */
class CARBONUI_DLL_API CarbonUIQToolBar : public QToolBar {
  public:
	/**
	 * @brief 构造工具栏并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQToolBar(QWidget* parent = nullptr);

	/**
	 * @brief 给已有工具栏应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] tb 目标工具栏，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QToolBar* tb);
};

#endif // CARBONUIQTOOLBAR_H
