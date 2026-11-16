/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQScrollArea.h
 * @brief 无边框滚动区控件：构造即获得 CarbonUI 暗色滚动区外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS
 * 与滚动条设置，调用方只需 new 后 setWidget()/加入布局。
 *
 * 外观契约：无边框、内容底色 \#3b3b3b；垂直与水平滚动条的外观统一委托给
 * CarbonUIQScrollBar（8px 命中范围、常态绘制 4px、悬停 8px、\#999999 滑块），
 * 细节见 `CarbonUIQScrollBar.h`。
 *
 * 需要给"外部已有的 QScrollArea"刷同样外观的调用方，可改用静态接口
 * CarbonUIQScrollArea::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQSCROLLAREA_H
#define CARBONUIQSCROLLAREA_H

#include "carbonui_global.h"

#include <QtWidgets/QScrollArea>

/**
 * @brief CarbonUI 无边框滚动区：自带暗色 QSS 的 QScrollArea 子类。
 */
class CARBONUI_DLL_API CarbonUIQScrollArea : public QScrollArea {
  public:
	/**
	 * @brief 构造滚动区并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQScrollArea(QWidget* parent = nullptr);

	/**
	 * @brief 给已有滚动区应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] area 目标滚动区，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QScrollArea* area);
};

#endif // CARBONUIQSCROLLAREA_H
