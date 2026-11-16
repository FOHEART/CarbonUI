/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTabWidget.h
 * @brief 标签页控件：构造即获得 CarbonUI 暗色标签页外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS
 * 装配，调用方只需 new 后加入布局并 addTab()。
 *
 * 外观契约：页签栏底 \#272727、选中页签底 \#2e2e2e + 文字 \#cdcdcd、内容面板
 * 底 \#3b3b3b + 1px \#2b2b2b 边框，页签栏右侧预留 66px 滚动区。
 *
 * 需要给"外部已有的 QTabWidget"（例如 Qt Designer 的 ui->tabWidget）刷同样
 * 外观的调用方，可改用静态接口 CarbonUIQTabWidget::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQTABWIDGET_H
#define CARBONUIQTABWIDGET_H

#include "carbonui_global.h"

#include <QtWidgets/QTabWidget>

/**
 * @brief CarbonUI 标签页：自带暗色 QSS 的 QTabWidget 子类。
 */
class CARBONUI_DLL_API CarbonUIQTabWidget : public QTabWidget {
  public:
	/**
	 * @brief 构造标签页并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQTabWidget(QWidget* parent = nullptr);

	/**
	 * @brief 给已有标签页控件应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] tw 目标标签页控件，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QTabWidget* tw);
};

#endif // CARBONUIQTABWIDGET_H
