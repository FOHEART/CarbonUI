/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQMenuBar.h
 * @brief 窗口菜单栏控件：构造即获得 CarbonUI 暗色菜单栏外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS、
 * 字体与几何设置，调用方只需 new 后交给 QMainWindow::setMenuBar()。
 *
 * 外观契约：关闭原生菜单栏、10pt 加粗、固定 26px 高，菜单底 \#3b3b3b、
 * 菜单项悬停/选中底 \#2e2e2e、弹出菜单底 \#2e2e2e。
 *
 * 需要给"外部已有的 QMenuBar"（例如 Qt Designer 的 ui->menuBar）刷同样外观
 * 的调用方，可改用静态接口 CarbonUIQMenuBar::applyStyle()（构造函数内部即调用它）。
 * 弹出 QMenu 不挂在菜单栏下（例如被重新 setParent 到别处）时，菜单栏样式表
 * 够不到它们，可用静态接口 CarbonUIQMenuBar::applyMenuStyle() 单独刷弹出菜单
 * 外观（子级子菜单会级联继承）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQMENUBAR_H
#define CARBONUIQMENUBAR_H

#include "carbonui_global.h"

#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>

class QMainWindow;

/**
 * @brief CarbonUI 菜单栏：自带暗色 QSS 的 QMenuBar 子类。
 */
class CARBONUI_DLL_API CarbonUIQMenuBar : public QMenuBar {
  public:
	/**
	 * @brief 构造菜单栏并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQMenuBar(QWidget* parent = nullptr);

	/**
	 * @brief 给已有菜单栏应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] b 目标菜单栏，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QMenuBar* b);

	/**
	 * @brief 给弹出菜单应用 CarbonUI 外观（菜单栏样式表中的 QMenu 部分）。
	 * @param[in,out] menu 目标弹出菜单，**可为 NULL**，为 NULL 时直接返回；
	 *                     其子级子菜单通过 QSS 级联继承同样外观。
	 * @note 同时把菜单字体设为菜单栏同款 10pt 加粗，保证弹出项观感一致。
	 */
	static void applyMenuStyle(QMenu* menu);

	/**
	 * @brief 基于 QMainWindow 现有（通常由 Qt Designer 生成）菜单栏装配
	 *        CarbonUI 菜单栏，并迁移全部菜单。
	 *
	 * @details 一步完成三件事，封装"替换 QMainWindow 菜单区"的完整套路：
	 * - **救出菜单**：旧栏的控件子对象（顶层 QMenu 等）重新挂到 @p window 下。
	 *   必须带原 windowFlags() 调 @c setParent()——顶层 QMenu 是 Qt::Popup
	 *   窗口，单参 setParent() 会剥掉窗口类型位，使 QMenu::popup() 失去鼠标
	 *   抓取而下拉菜单一闪即关；
	 * - **迁移 action**：旧栏的 action 全部迁到新栏（保持顺序），新栏沿用旧栏
	 *   的 objectName；
	 * - **统一外观**：对新栏调用 applyStyle()，对救出的顶层菜单调用
	 *   applyMenuStyle()——菜单被重新挂父后已不在旧栏样式表作用域内，不刷会
	 *   退回系统默认白底。
	 *
	 * 调用方拿到新栏后自行放进菜单宿主（例如与 CarbonUIQTitleBar 一起放入
	 * `QMainWindow::setMenuWidget()` 的布局）；旧栏随后随宿主替换被销毁，本
	 * 接口不删除它。
	 *
	 * @param[in,out] window 目标主窗口，**可为 NULL**，为 NULL 时返回 NULL。
	 * @return 装配好的新菜单栏（已应用 CarbonUI 外观），挂在 @p window 下。
	 */
	static QMenuBar* adoptExistingMenus(QMainWindow* window);

	/**
	 * @brief 以菜单栏字体为唯一真相源，统一栏上全部主菜单 / 子菜单与菜单项的字体。
	 *
	 * @details 从 @p bar 的 action 递归可达的每一个 QMenu 都显式覆盖为菜单栏
	 * 同款字体（10pt 加粗）：菜单对象与它包含的每个菜单项 action 一并设置。
	 * 这样做是因为 Qt Designer 常在 .ui 里混设字号（如菜单 11pt / 菜单项
	 * 10pt），而 QMenu 的显式字体挡住 Qt 的父到子字体传播，动态创建（无显式
	 * 字体）的子菜单又只继承到应用默认字体——两路混排导致菜单字号不一。
	 * 收集走「action → menu → 子菜单」递归而不是 bar->findChildren()：
	 * 被 adoptExistingMenus() 救出的顶层菜单挂在主窗口下、不在菜单栏子树内；
	 * 动态 insertMenu() 挂入的子菜单也常以主窗口为父，因此同时按 action
	 * 与直接子控件两条路径收集，带 visited 去重防环。
	 *
	 * @param[in,out] bar 目标菜单栏，**可为 NULL**，为 NULL 时直接返回。
	 * @note 在全部菜单创建完成后调用一次即可（运行期后建的子菜单需再调一次）。
	 */
	static void unifyFonts(QMenuBar* bar);
};

#endif // CARBONUIQMENUBAR_H
