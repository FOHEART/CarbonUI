/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQScrollBar.cpp
 * @brief CarbonUIQScrollBar 的外观实现（外观契约见 CarbonUIQScrollBar.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 全部外观写在一条 QSS 字面量里：常态绘制厚度 4px、悬停 8px，轨道 \#2e2e2e、
 * 滑块 \#999999，配色取自 tools/reference/滚动条.png 与 滚动条HOVER.png 的像素
 * 取样（两张图的差异只在滚动条列的厚度，颜色不变）。
 *
 * 命中范围与绘制厚度分开：控件横向（垂直条为宽、水平条为高）固定占 8px，
 * 常态靠 QSS `margin` 在两侧各留 2px 透明留白，只画中间 4px；悬停时 `margin`
 * 归零、整条 8px 上色。这样鼠标的点击/拖拽区域始终有 8px 宽（比绘制出来的细条
 * 更好点），悬停加厚不需要改变控件几何、不会让内容抖动。
 *
 * QSS 里同时写垂直与水平两组规则：`:hover` 加厚由 QSS 伪态直接驱动，不需要
 * 任何事件代码；无箭头按钮通过把 add-line/sub-line 压到 0 并清空箭头子控件
 * 实现，无加减页色块通过 add-page/sub-page 透明实现——轨道因此呈现为连续的
 * 一条 #2e2e2e。一个滚动条只按自身方向命中其中一组规则，所以同一条 QSS 可以
 * 安全地刷给任意方向的 QScrollBar。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQScrollBar.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace {

/// 命中范围与悬停绘制厚度（像素），垂直=宽 / 水平=高。
constexpr int kBarThickness = 8;
/// 常态绘制厚度（像素）：只画中间这段，两侧留白归入命中范围。
constexpr int kRestThickness = 4;
/// 常态在厚度方向两侧各留的透明留白（像素）：kBarThickness 与 kRestThickness 之差的一半。
constexpr int kRestInset = (kBarThickness - kRestThickness) / 2;
/// 滑块最短长度（像素），防止内容远大于视口时滑块小到无法拖拽。
constexpr int kHandleMinLength = 20;

/**
 * @brief 拼一个方向的完整 QSS 规则组（轨道 / 滑块 / 箭头 / 加减页）。
 *
 * @param[in] orientation 方向后缀，取 "vertical" 或 "horizontal"，不可为 NULL。
 * @return 该方向的全部规则；垂直条靠 width 定命中范围、margin 定常态绘制厚度，
 *         滑块受 min-height 约束；水平条对调为 height / min-width。
 */
QString orientationRules(const char* orientation) {
	const QString o = QString::fromLatin1(orientation);
	const bool vertical = qstrcmp(orientation, "vertical") == 0;
	const QString thicknessProperty = vertical ? QStringLiteral("width") : QStringLiteral("height");
	const QString lengthProperty = vertical ? QStringLiteral("height") : QStringLiteral("width");
	const QString lineSizeProperty = vertical ? QStringLiteral("height") : QStringLiteral("width");
	// 常态只画中间 kRestThickness：垂直条左右留白，水平条上下留白
	const QString restMargin = vertical ? QStringLiteral("0px %1px").arg(kRestInset)
	                                    : QStringLiteral("%1px 0px").arg(kRestInset);

	QString rules = QStringLiteral("QScrollBar:%1 { background: #2e2e2e; %2: %3px;"
	                               " margin: %4; border: none; }")
	                    .arg(o, thicknessProperty)
	                    .arg(kBarThickness)
	                    .arg(restMargin);
	// 悬停：留白归零，整条命中范围都上色（厚度翻倍，控件几何不变）
	rules += QStringLiteral("QScrollBar:%1:hover { margin: 0px; }").arg(o);
	rules += QStringLiteral("QScrollBar::handle:%1 { background: #999999; min-%2: %3px;"
	                        " margin: 0px; }")
	             .arg(o, lengthProperty)
	             .arg(kHandleMinLength);
	rules += QStringLiteral("QScrollBar::sub-line:%1, QScrollBar::add-line:%1 { %2: 0px;"
	                        " background: none; }")
	             .arg(o, lineSizeProperty);
	rules += QStringLiteral("QScrollBar::up-arrow:%1, QScrollBar::down-arrow:%1"
	                        " { width: 0px; height: 0px; background: none; }")
	             .arg(o);
	rules += QStringLiteral("QScrollBar::add-page:%1,"
	                        " QScrollBar::sub-page:%1 { background: none; }")
	             .arg(o);
	return rules;
}

} // namespace

/**
 * @brief 构造滚动条并应用 CarbonUI 外观（详见 CarbonUIQScrollBar.h）。
 */
CarbonUIQScrollBar::CarbonUIQScrollBar(Qt::Orientation orientation, QWidget* parent)
    : QScrollBar(orientation, parent) {
	applyStyle(this);
}

/**
 * @brief 给已有滚动条应用 CarbonUI 外观（详见 CarbonUIQScrollBar.h）。
 */
void CarbonUIQScrollBar::applyStyle(QScrollBar* bar) {
	if (bar == nullptr) {
		return;
	}

	bar->setStyleSheet(orientationRules("vertical") + orientationRules("horizontal"));
}
