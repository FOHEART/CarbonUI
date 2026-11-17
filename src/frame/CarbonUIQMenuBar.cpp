/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQMenuBar.cpp
 * @brief CarbonUIQMenuBar 的外观实现（外观契约见 CarbonUIQMenuBar.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 关闭原生菜单栏（跨平台外观一致的前提）、10pt 加粗、固定 26px 高；菜单栏
 * 与弹出菜单共用同一个底色常量，改配色只需改一处。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQMenuBar.h"
#include <QtGui/QFont>

/**
 * @brief 构造菜单栏并完成外观装配（详见 CarbonUIQMenuBar.h）。
 */
CarbonUIQMenuBar::CarbonUIQMenuBar(QWidget* parent) : QMenuBar(parent) {
	applyStyle(this);
}

/**
 * @brief 给已有菜单栏应用 CarbonUI 外观（详见 CarbonUIQMenuBar.h）。
 */
void CarbonUIQMenuBar::applyStyle(QMenuBar* b) {
	if (b == nullptr) {
		return;
	}

	// central color for menu background/border so it can be changed easily
	QString menuBg = "#3b3b3b";

	b->setNativeMenuBar(false);
	QFont mbFont = b->font();
	mbFont.setPointSize(10);
	mbFont.setBold(true);
	b->setFont(mbFont);
	b->setFixedHeight(26);

	QString style = QString("QMenuBar { background: %1; color: #cdcdcd; spacing: 0px; padding: 2px "
	                        "0px; margin: 0px; }"
	                        "QMenuBar::item { background: transparent; padding: 2px 10px; margin: "
	                        "0px; border-radius: 0px; }"
	                        "QMenuBar::item:hover { background: #2e2e2e;  color: #cdcdcd; }"
	                        "QMenuBar::item:selected { background: #2e2e2e; color: #cdcdcd; }"
	                        "QMenu { background: #2e2e2e; color: #d0d0d0; border: 1px solid %1; }"
	                        "QMenu::item:selected { background: %1; color: #cdcdcd; }")
	                    .arg(menuBg);

	b->setStyleSheet(style);
}
