/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTreeWidget.h
 * @brief 表格型树控件：构造即获得 CarbonUI 暗色树控件外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS、
 * 字体与表头设置，调用方只需 new 后加入布局并填充列。
 *
 * 外观契约：压平缩进（indentation = 0）、关闭根节点装饰、整行选中色
 * \#0078d7，表头左对齐、最小高度 24px、13pt 加粗，正文 13pt 常规字体。
 *
 * 需要给"外部已有的 QTreeWidget"刷同样外观的调用方，可改用静态接口
 * CarbonUIQTreeWidget::applyStyle()（构造函数内部即调用它）。层级型树视图请
 * 使用 CarbonUIQTreeView。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQTREEWIDGET_H
#define CARBONUIQTREEWIDGET_H

#include "carbonui_global.h"

#include <QtWidgets/QTreeWidget>

/**
 * @brief CarbonUI 表格型树控件：自带暗色 QSS 的 QTreeWidget 子类。
 */
class CARBONUI_DLL_API CarbonUIQTreeWidget : public QTreeWidget {
  public:
	/**
	 * @brief 构造树控件并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQTreeWidget(QWidget* parent = nullptr);

	/**
	 * @brief 给已有树控件应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] tree 目标树控件，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QTreeWidget* tree);
};

#endif // CARBONUIQTREEWIDGET_H
