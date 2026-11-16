/**
 * @file  carbonui_scrollbar_test.cpp
 * @brief `CarbonUIQScrollBar` 细滚动条样式的行为用例（配置层）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 外观契约来自 tools/reference/滚动条.png 与 滚动条HOVER.png 的像素取样：常态
 * 绘制厚度 4px、悬停 8px，轨道 \#2e2e2e、滑块 \#999999，无箭头按钮与加减页
 * 色块。命中范围固定 8px（常态两侧各留 2px 透明留白，只画中间 4px）。本文件
 * 断言"配置"（QSS 规则、命中范围、方向、空指针容错）；"画出来的像素"（轨道与
 * 滑块真实颜色、常态确实只画 4px）由 tests/visual/carbonui_render_test.cpp 的
 * ScrollBar 用例覆盖。
 */

#include <gtest/gtest.h>

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>

#include "CarbonUIQScrollBar.h"
#include "qt_test_support.h"

TEST(ScrollBarStyle, NullWidgetIsIgnored) {
	EXPECT_NO_FATAL_FAILURE(CarbonUIQScrollBar::applyStyle(nullptr));
}

TEST(ScrollBarStyle, VerticalBarMatchesReferenceMetrics) {
	QScrollBar bar(Qt::Vertical);

	CarbonUIQScrollBar::applyStyle(&bar);

	// 命中范围 8px，常态两侧各留 2px → 只画中间 4px；颜色取自参考图取样
	EXPECT_QSS_CONTAINS(bar.styleSheet(), "QScrollBar:vertical { background: #2e2e2e; width: 8px;"
	                                      " margin: 0px 2px; border: none; }");
	// 悬停：留白归零，整条命中范围上色（4px → 8px）
	EXPECT_QSS_CONTAINS(bar.styleSheet(), "QScrollBar:vertical:hover { margin: 0px; }");
	EXPECT_QSS_CONTAINS(bar.styleSheet(),
	                    "QScrollBar::handle:vertical { background: #999999; min-height: 20px;");
	// 无箭头按钮、无加减页色块：轨道因此呈现为连续的一条
	EXPECT_QSS_CONTAINS(bar.styleSheet(),
	                    "QScrollBar::sub-line:vertical, QScrollBar::add-line:vertical"
	                    " { height: 0px;");
	EXPECT_QSS_CONTAINS(
	    bar.styleSheet(),
	    "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
}

TEST(ScrollBarStyle, HitAreaIsWiderThanPaintedStrip) {
	QScrollBar bar(Qt::Vertical);

	CarbonUIQScrollBar::applyStyle(&bar);

	// 命中范围来自 QSS width（8px），与常态绘制厚度（8 − 2×2 = 4px）解耦：
	// 鼠标不必压在 4px 细条上也能抓住滚动条
	EXPECT_EQ(bar.sizeHint().width(), 8)
	    << "the hit area must stay 8px wide so the thin bar is easy to grab";
	EXPECT_EQ(bar.minimumWidth(), 0)
	    << "thickness must come from QSS so :hover can grow it, not from setMinimumWidth";
}

TEST(ScrollBarStyle, HorizontalBarMirrorsVerticalMetrics) {
	QScrollBar bar(Qt::Horizontal);

	CarbonUIQScrollBar::applyStyle(&bar);

	// 水平条与垂直条同一套契约，只是厚度属性对调（height / 上下留白）
	EXPECT_QSS_CONTAINS(bar.styleSheet(),
	                    "QScrollBar:horizontal { background: #2e2e2e; height: 8px;"
	                    " margin: 2px 0px; border: none; }");
	EXPECT_QSS_CONTAINS(bar.styleSheet(), "QScrollBar:horizontal:hover { margin: 0px; }");
	EXPECT_QSS_CONTAINS(bar.styleSheet(),
	                    "QScrollBar::handle:horizontal { background: #999999; min-width: 20px;");
	EXPECT_QSS_CONTAINS(bar.styleSheet(),
	                    "QScrollBar::sub-line:horizontal, QScrollBar::add-line:horizontal"
	                    " { width: 0px;");
}

TEST(ScrollBarStyle, OneStyleSheetServesBothOrientations) {
	QScrollBar vertical(Qt::Vertical);
	QScrollBar horizontal(Qt::Horizontal);

	CarbonUIQScrollBar::applyStyle(&vertical);
	CarbonUIQScrollBar::applyStyle(&horizontal);

	// QSS 同时包含两组方向规则：刷给任一方向都完整，外部滚动条无需关心方向
	EXPECT_QSS_CONTAINS(vertical.styleSheet(),
	                    "QScrollBar:horizontal { background: #2e2e2e; height: 8px;");
	EXPECT_QSS_CONTAINS(horizontal.styleSheet(),
	                    "QScrollBar:vertical { background: #2e2e2e; width: 8px;");
}

TEST(ScrollBarStyle, ConstructorAppliesStyleWithoutTouchingOrientation) {
	CarbonUIQScrollBar bar(Qt::Vertical);

	EXPECT_EQ(bar.orientation(), Qt::Vertical);
	EXPECT_QSS_CONTAINS(bar.styleSheet(), "QScrollBar::handle:vertical { background: #999999;");
}
