/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQToolBar.cpp
 * @brief CarbonUIQToolBar 的外观实现（外观契约见 CarbonUIQToolBar.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 禁止拖动/浮动、固定 32px 高、28px 图标；按钮间距由布局 spacing 与左内容
 * 边距共同决定。
 *
 * @note @c setIconSize() 会清掉 @c setFixedHeight() 设置的最小高度，
 *       因此高度契约只体现在 @c maximumHeight() == 32 上。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQToolBar.h"
#include <QtCore/QSize>
#include <QtWidgets/QLayout>

/**
 * @brief 构造工具栏并完成外观装配（详见 CarbonUIQToolBar.h）。
 */
CarbonUIQToolBar::CarbonUIQToolBar(QWidget* parent) : QToolBar(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有工具栏应用 CarbonUI 外观（详见 CarbonUIQToolBar.h）。
 */
void CarbonUIQToolBar::applyStyle(QToolBar* tb) {
	if (tb == nullptr) {
		return;
	}

	// set toolbar icon size and add down icon
	tb->setMovable(false);
	tb->setFloatable(false);
	tb->setFixedHeight(32);
	tb->setIconSize(QSize(28, 28));

	int spacing = 2;
	QString tbStyle = QString("QToolBar { background: #3b3b3b;  border-top: 1px solid #212121; }"
	                          "QToolBar::handle { width: 0px; }"
	                          "QToolButton { border: none; border-radius: 10px; padding: 2px; }"
	                          "QToolButton:hover { border: 1px solid #5d5d5d; }");

	tb->setStyleSheet(tbStyle);

	QLayout* tbLayout = tb->layout();
	if (tbLayout != nullptr) {
		tbLayout->setSpacing(spacing);
	}

	tb->setContentsMargins(32, 0, spacing / 2, 0);
}
