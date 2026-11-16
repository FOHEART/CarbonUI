/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQMenuBar.h
 * @brief 窗口菜单栏控件：构造即获得 CarbonUI 暗色菜单栏外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS、
 * 字体与几何设置，调用方只需 new 后交给 QMainWindow::setMenuBar()。
 *
 * 外观契约：关闭原生菜单栏、10pt 加粗、固定 26px 高，菜单底 \#3b3b3b、
 * 菜单项悬停/选中底 \#2e2e2e、弹出菜单底 \#2e2e2e。
 *
 * 需要给"外部已有的 QMenuBar"（例如 Qt Designer 的 ui->menuBar）刷同样外观
 * 的调用方，可改用静态接口 CarbonUIQMenuBar::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQMENUBAR_H
#define CARBONUIQMENUBAR_H

#include "carbonui_global.h"

#include <QtWidgets/QMenuBar>

/**
 * @brief CarbonUI 菜单栏：自带暗色 QSS 的 QMenuBar 子类。
 */
class CARBONUI_DLL_API CarbonUIQMenuBar : public QMenuBar {
  public:
	/**
	 * @brief 构造菜单栏并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQMenuBar(QWidget* parent = nullptr);

	/**
	 * @brief 给已有菜单栏应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] b 目标菜单栏，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QMenuBar* b);
};

#endif // CARBONUIQMENUBAR_H
