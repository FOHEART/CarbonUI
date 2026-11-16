/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTreeView.h
 * @brief 层级型树视图控件：构造即获得 CarbonUI 暗色树视图外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS
 * 装配，调用方只需 new 后 setModel()/加入布局。
 *
 * 外观契约：保留 14px 缩进与展开箭头（箭头使用资源图片），选中底 \#2e2e2e
 * + 白字，禁止编辑。
 *
 * @note 展开/收起箭头的图片路径为 `:/images/...`，与 `carbonui_rcc.qrc` 的
 *       `/img` 前缀不一致，资源不存在时箭头不显示（不影响其它属性）。
 *
 * 需要给"外部已有的 QTreeView"（含 QTreeWidget 等子类实例）刷同样外观的
 * 调用方，可改用静态接口 CarbonUIQTreeView::applyStyle()（构造函数内部即调用
 * 它）。表格型树控件请使用 CarbonUIQTreeWidget。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQTREEVIEW_H
#define CARBONUIQTREEVIEW_H

#include "carbonui_global.h"

#include <QtWidgets/QTreeView>

/**
 * @brief CarbonUI 层级型树视图：自带暗色 QSS 的 QTreeView 子类。
 */
class CARBONUI_DLL_API CarbonUIQTreeView : public QTreeView {
  public:
	/**
	 * @brief 构造树视图并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQTreeView(QWidget* parent = nullptr);

	/**
	 * @brief 给已有树视图应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] view 目标树视图，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QTreeView* view);
};

#endif // CARBONUIQTREEVIEW_H
