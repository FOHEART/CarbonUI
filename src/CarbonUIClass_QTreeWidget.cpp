/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QTreeWidget.cpp
 * @brief CarbonUIClass 中表格型树控件与层级型树视图样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 两个树类助手共置一文件：
 * - @ref CarbonUIClass::applyTreeWidgetStyle：压平缩进、加粗表头、整行选中色；
 * - @ref CarbonUIClass::applyTreeViewStyle：保留 14px 缩进与展开箭头
 *   （箭头图片路径 `:/images/...`，与 `carbonui_rcc.qrc` 的 `/img` 前缀不一致，
 *   资源不存在时箭头不显示），禁止编辑。
 * 公开接口的完整参数说明写在 CarbonUIClass.h；本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtGui/QFont>
#include <QtWidgets/QHeaderView>


// helper to apply consistent styling to a QTreeWidget
 void CarbonUIClass::applyTreeWidgetStyle(QTreeWidget* tree)
{
	if (!tree)
		return;

	// style tree widget: outer border 2px solid #5f5f5f, header background and text, header bottom inner border 1px
	tree->setStyleSheet(
		"QTreeWidget { border: 2px solid #5f5f5f; background-color: #2f2f2f; color: #cdcdcd; }"
		"QHeaderView::section { background-color: #3b3b3b; color: #ffffff; border: 1px solid #5f5f5f; font-weight: bold; }"
		"QTreeWidget::item { border: 1px solid #5f5f5f; padding-left: 0px; min-height: 24px; }"
		"QTreeWidget::item:selected, QTreeView::item:selected { background-color: #0078d7; color: #ffffff; }"
	);

	// remove left indentation/spacing for first column and align header left
	tree->setIndentation(0);
	tree->setRootIsDecorated(false);

	QFont mbFont = tree->font();
	mbFont.setPointSize(13);
	mbFont.setBold(false);
	if (tree->header())
	{
		tree->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		tree->header()->setMinimumHeight(24);
		mbFont.setBold(true);
		tree->header()->setFont(mbFont);
	}

	mbFont.setBold(false);
	tree->setFont(mbFont);
}

// helper to apply consistent styling to a QTreeView (or subclasses)
void CarbonUIClass::applyTreeViewStyle(QTreeView* view)
{
	if (!view)
		return;

	// Use resource images for branch indicators
    view->setStyleSheet(R"(
            QTreeView { background-color: transparent; color: white; border: none; }
            QTreeView::item { color: white; border: none; }
            QTreeView::item:selected { background-color: #2e2e2e; color: white; border: none; }
            QTreeView::item:selected:active { background-color: #2e2e2e; color: white; }
            QTreeView::item:focus { outline: none; }
            QTreeView::branch { background: transparent; }
            /* Closed (right-pointing) arrow - white image resource */
            QTreeView::branch:closed:has-children {
                image: url(":/images/Resources/img/arrow_right.png");
                width: 24px; height: 24px; }
            /* Open (down-pointing) arrow - white image resource */
            QTreeView::branch:open:has-children {
                image: url(":/images/Resources/img/arrow_down.png");
                width: 24px; height: 24px; }
        )");

	// Ensure the view shows branch indicators and can expand
	view->setRootIsDecorated(true);
	view->setItemsExpandable(true);
	view->setExpandsOnDoubleClick(true);
	// Ensure there is space for the branch icon
    view->setIndentation(14);

	// Disable editing via the view
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
}
