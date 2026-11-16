/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QMenuBar.cpp
 * @brief CarbonUIClass 中窗口菜单栏样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::setupMenuBarStyle ——关闭原生菜单栏、10pt 加粗、
 * 固定 26px 高与深底配色。公开接口的完整参数说明写在 CarbonUIClass.h；
 * 本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtGui/QFont>


void CarbonUIClass::setupMenuBarStyle(QMenuBar* b)
{
	if (!b)
		return;

	// central color for menu background/border so it can be changed easily
	QString menuBg = "#3b3b3b";

	b->setNativeMenuBar(false);
	QFont mbFont = b->font();
	mbFont.setPointSize(10);
	mbFont.setBold(true);
	b->setFont(mbFont);
	b->setFixedHeight(26);

	QString style = QString(
		"QMenuBar { background: %1; color: #cdcdcd; spacing: 0px; padding: 2px 0px; margin: 0px; }"
		"QMenuBar::item { background: transparent; padding: 2px 10px; margin: 0px; border-radius: 0px; }"
		"QMenuBar::item:hover { background: #2e2e2e;  color: #cdcdcd; }"
		"QMenuBar::item:selected { background: #2e2e2e; color: #cdcdcd; }"
		"QMenu { background: #2e2e2e; color: #d0d0d0; border: 1px solid %1; }"
		"QMenu::item:selected { background: %1; color: #cdcdcd; }"
	).arg(menuBg);

	b->setStyleSheet(style);
}
