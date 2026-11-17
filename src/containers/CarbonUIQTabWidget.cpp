/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTabWidget.cpp
 * @brief CarbonUIQTabWidget 的外观实现（外观契约见 CarbonUIQTabWidget.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 页签栏右侧未占用的滚动区会露出宿主调色板的浅色，因此 QTabWidget 自身与
 * QTabBar 的底色都必须显式压成页签底色（app 截图基线 app-tab-* 覆盖此行为）。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQTabWidget.h"
#include <QtWidgets/QTabBar>

/**
 * @brief 构造标签页并完成外观装配（详见 CarbonUIQTabWidget.h）。
 */
CarbonUIQTabWidget::CarbonUIQTabWidget(QWidget* parent) : QTabWidget(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有标签页控件应用 CarbonUI 外观（详见 CarbonUIQTabWidget.h）。
 */
void CarbonUIQTabWidget::applyStyle(QTabWidget* tw) {
	if (tw == nullptr) {
		return;
	}

	QString style =
	    // QTabWidget 自身也要给底色：页签栏不铺满全宽时，右侧露出的是宿主控件的
	    // palette Window 色（浅色），必须压成页签底色。
	    "QTabWidget { background-color: #272727; }"
	    "QTabWidget::pane { background: #3b3b3b; border-radius: 0px; border: 1px solid #2b2b2b; }"
	    // 页签栏自身的底色必须显式给：否则右侧未占用的滚动区会露出 palette 的浅色，
	    // 在深色主题里是一块刺眼的白条（已由 app 截图基线 app-tab-* 覆盖）。
	    "QTabBar { background-color: #272727; padding-right: 66px; }"
	    "QTabBar::tab { background: #272727; color: #575757; padding: 8px 20px; margin-right: 0px;"
	    " margin-top: 0px; border: none; }"
	    "QTabBar::tab:selected { background: #2e2e2e; color: #cdcdcd; }"
	    "QTabBar::scroller { background-color: transparent; }"
	    "QTabBar::scroller QToolButton { background-color: #3b3b3b; color: #cdcdcd; border: none;"
	    " padding: 0px; margin: 0px; min-width: 22px; min-height: 22px; border-radius: 0px; }";

	tw->setStyleSheet(style);
}
