/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTreeWidget.cpp
 * @brief CarbonUIQTreeWidget 的外观实现（外观契约见 CarbonUIQTreeWidget.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 表格型树控件外观：外框 2px \#5f5f5f、内容底 \#2f2f2f、整行选中色
 * \#0078d7；压平缩进（indentation = 0）、关闭根节点装饰，表头左对齐、
 * 最小高度 24px、13pt 加粗，正文 13pt 常规字体。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQTreeWidget.h"
#include <QtGui/QFont>
#include <QtWidgets/QHeaderView>

/**
 * @brief 构造树控件并完成外观装配（详见 CarbonUIQTreeWidget.h）。
 */
CarbonUIQTreeWidget::CarbonUIQTreeWidget(QWidget* parent) : QTreeWidget(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有树控件应用 CarbonUI 外观（详见 CarbonUIQTreeWidget.h）。
 */
void CarbonUIQTreeWidget::applyStyle(QTreeWidget* tree) {
	if (tree == nullptr) {
		return;
	}

	// style tree widget: outer border 2px solid #5f5f5f, header background and text,
	// header bottom inner border 1px
	tree->setStyleSheet(
	    "QTreeWidget { border: 2px solid #5f5f5f; background-color: #2f2f2f; color: #cdcdcd; }"
	    "QHeaderView::section { background-color: #3b3b3b; color: #ffffff; border: 1px solid "
	    "#5f5f5f;"
	    " font-weight: bold; }"
	    "QTreeWidget::item { border: 1px solid #5f5f5f; padding-left: 0px; min-height: 24px; }"
	    "QTreeWidget::item:selected, QTreeView::item:selected { background-color: #0078d7; color: "
	    "#ffffff; }");

	// remove left indentation/spacing for first column and align header left
	tree->setIndentation(0);
	tree->setRootIsDecorated(false);

	QFont headerFont = tree->font();
	headerFont.setPointSize(13);
	headerFont.setBold(false);
	QHeaderView* header = tree->header();
	if (header != nullptr) {
		header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		header->setMinimumHeight(24);
		headerFont.setBold(true);
		header->setFont(headerFont);
	}

	headerFont.setBold(false);
	tree->setFont(headerFont);
}
