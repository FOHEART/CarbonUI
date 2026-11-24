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
#include <QtCore/QSet>
#include <QtGui/QFont>
#include <QtWidgets/QAction>
#include <QtWidgets/QMainWindow>

namespace {

/**
 * @brief 弹出菜单 QSS（与菜单栏样式表中的 QMenu 部分保持同一来源）。
 */
QString menuPopupStyle(const QString& menuBg) {
	return QString("QMenu { background: #2e2e2e; color: #d0d0d0; border: 1px solid %1; }"
	               "QMenu::item:selected { background: %1; color: #cdcdcd; }")
	    .arg(menuBg);
}

/**
 * @brief 菜单栏底色（菜单栏与弹出菜单共用，改配色只改这里）。
 */
QString menuCentralColor() {
	return QStringLiteral("#3b3b3b");
}

} // namespace

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

	QString menuBg = menuCentralColor();

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
	                        "%2")
	                    .arg(menuBg, menuPopupStyle(menuBg));

	b->setStyleSheet(style);
}

/**
 * @brief 给弹出菜单应用 CarbonUI 外观（详见 CarbonUIQMenuBar.h）。
 */
void CarbonUIQMenuBar::applyMenuStyle(QMenu* menu) {
	if (menu == nullptr) {
		return;
	}

	QFont menuFont = menu->font();
	menuFont.setPointSize(10);
	menuFont.setBold(true);
	menu->setFont(menuFont);
	menu->setStyleSheet(menuPopupStyle(menuCentralColor()));
}

/**
 * @brief 基于现有菜单栏装配 CarbonUI 菜单栏并迁移菜单（详见 CarbonUIQMenuBar.h）。
 */
QMenuBar* CarbonUIQMenuBar::adoptExistingMenus(QMainWindow* window) {
	if (window == nullptr) {
		return nullptr;
	}

	QMenuBar* oldBar = window->menuBar();
	QList<QMenu*> rescuedMenus;
	if (oldBar != nullptr) {
		// 救出挂在旧栏下的顶层 QMenu（必须带 windowFlags() 调 setParent()，
		// 否则 Qt::Popup 窗口类型位被剥掉，下拉菜单一闪即关）。
		const QList<QObject*> oldChildren = oldBar->children();
		for (QObject* child : oldChildren) {
			auto* childWidget = qobject_cast<QWidget*>(child);
			if (childWidget == nullptr) {
				continue;
			}
			childWidget->setParent(window, childWidget->windowFlags());
			if (auto* rescuedMenu = qobject_cast<QMenu*>(childWidget)) {
				rescuedMenus.append(rescuedMenu);
			}
		}
	}

	auto* newBar = new QMenuBar(window);
	newBar->setObjectName(oldBar != nullptr && !oldBar->objectName().isEmpty()
	                          ? oldBar->objectName()
	                          : QStringLiteral("menuBar"));
	if (oldBar != nullptr) {
		const QList<QAction*> actions = oldBar->actions();
		for (QAction* action : actions) {
			newBar->addAction(action);
		}
	}

	applyStyle(newBar);
	// 顶层菜单已被重新挂到 window 下，旧栏样式表够不到它们；单独刷 CarbonUI
	// 弹出菜单外观，否则退回系统默认白底。
	for (QMenu* rescuedMenu : rescuedMenus) {
		applyMenuStyle(rescuedMenu);
	}
	return newBar;
}

/**
 * @brief 递归统一一棵菜单子树内全部菜单与菜单项的字体。
 */
namespace {

void unifyMenuSubtreeFonts(QMenu* menu, const QFont& font, QSet<QMenu*>& visited) {
	if (menu == nullptr || visited.contains(menu)) {
		return;
	}
	visited.insert(menu);
	menu->setFont(font);

	QList<QMenu*> submenus;
	const QList<QAction*> actions = menu->actions();
	for (QAction* action : actions) {
		action->setFont(font);
		// 动态子菜单可能以主窗口为父（如 insertMenu 挂入的「视图」），只能通过
		// action 到达，不能只靠 findChildren 沿 Qt 父子链查找。
		if (auto* submenu = action->menu()) {
			submenus.append(submenu);
		}
	}
	const QList<QMenu*> childMenus =
	    menu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
	for (QMenu* child : childMenus) {
		if (!submenus.contains(child)) {
			submenus.append(child);
		}
	}
	for (QMenu* submenu : submenus) {
		unifyMenuSubtreeFonts(submenu, font, visited);
	}
}

} // namespace

/**
 * @brief 统一栏上全部主菜单 / 子菜单与菜单项的字体（详见 CarbonUIQMenuBar.h）。
 */
void CarbonUIQMenuBar::unifyFonts(QMenuBar* bar) {
	if (bar == nullptr) {
		return;
	}

	const QFont font = bar->font();
	QSet<QMenu*> visited;
	const QList<QAction*> actions = bar->actions();
	for (QAction* action : actions) {
		action->setFont(font);
		unifyMenuSubtreeFonts(action->menu(), font, visited);
	}
}
