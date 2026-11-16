/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQScrollBar.h
 * @brief 细滚动条控件：构造即获得 CarbonUI 暗色滚动条外观（垂直 / 水平同一套样式）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIQScrollArea 同一实现范式：控件自己负责外观，构造函数内完成全部
 * QSS 设置，调用方只需 new 后交给布局或 QAbstractScrollArea。
 *
 * 外观契约（取自 tools/reference/滚动条.png 与 tools/reference/滚动条HOVER.png
 * 的像素取样）：
 * - 命中范围：厚度方向固定占 8px（垂直=宽、水平=高），比画出来的细条更宽，
 *   鼠标容易点到；悬停加厚只改绘制、不改控件几何，内容不会抖动；
 * - 常态：在命中范围内只绘制中间 4px（两侧各 2px 透明留白），轨道 \#2e2e2e，
 *   滑块 \#999999，方角，无上下/左右箭头按钮，无加减页色块；
 * - 悬停：留白归零，整条 8px 上色（厚度翻倍），轨道与滑块颜色不变；
 * - 垂直与水平共用一条 QSS，方向规则互不干扰；一个滚动条只会命中自身方向的
 *   规则，因此 @c applyStyle() 可以安全地刷给任意方向的 QScrollBar。
 *
 * 需要给"外部已有的 QScrollBar"刷同样外观的调用方（例如 QScrollArea /
 * QTreeView / QPlainTextEdit 的自带滚动条），可改用静态接口
 * CarbonUIQScrollBar::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQSCROLLBAR_H
#define CARBONUIQSCROLLBAR_H

#include "carbonui_global.h"

#include <QtWidgets/QScrollBar>

/**
 * @brief CarbonUI 细滚动条：自带暗色 QSS 的 QScrollBar 子类。
 */
class CARBONUI_DLL_API CarbonUIQScrollBar : public QScrollBar {
  public:
	/**
	 * @brief 构造滚动条并应用 CarbonUI 外观。
	 * @param[in] orientation 滚动条方向，取 Qt::Horizontal 或 Qt::Vertical。
	 * @param[in] parent      父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

	/**
	 * @brief 给已有滚动条应用 CarbonUI 外观（不改变对象类型与方向）。
	 *
	 * @param[in,out] bar 目标滚动条，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)；样式表同时包含垂直与
	 *       水平规则，滚动条只按自身方向命中其中一组。
	 */
	static void applyStyle(QScrollBar* bar);
};

#endif // CARBONUIQSCROLLBAR_H
