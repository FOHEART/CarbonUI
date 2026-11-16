/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTreeView.cpp
 * @brief CarbonUIQTreeView 的外观实现（外观契约见 CarbonUIQTreeView.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 层级型树视图外观：保留 14px 缩进与展开/收起箭头（箭头图片路径
 * `:/images/...`，与 `carbonui_rcc.qrc` 的 `/img` 前缀不一致，资源不存在时
 * 箭头不显示），选中底 \#2e2e2e + 白字，禁止编辑。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQTreeView.h"

/**
 * @brief 构造树视图并完成外观装配（详见 CarbonUIQTreeView.h）。
 */
CarbonUIQTreeView::CarbonUIQTreeView(QWidget* parent) : QTreeView(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有树视图应用 CarbonUI 外观（详见 CarbonUIQTreeView.h）。
 */
void CarbonUIQTreeView::applyStyle(QTreeView* view) {
	if (view == nullptr) {
		return;
	}

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
