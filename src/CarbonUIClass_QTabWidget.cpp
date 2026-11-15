/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QTabWidget.cpp
 * @brief CarbonUIClass 中标签页样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::setupTabWidgetStyle：面板底色、选中页签高亮、
 * 右侧预留 66px 滚动区。公开接口的完整参数说明写在 CarbonUIClass.h；
 * 本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"


void CarbonUIClass::setupTabWidgetStyle(QTabWidget* tw)
{
    if (!tw) return;

    QString style =
        "QTabWidget::pane { background: #3b3b3b; border-radius: 0px; border: 1px solid #2b2b2b; }"
        "QTabBar { padding-right: 66px; }"
        "QTabBar::tab { background: #272727; color: #575757; padding: 8px 20px; margin-right: 0px; margin-top: 0px; border: none; }"
        "QTabBar::tab:selected { background: #2e2e2e; color: #cdcdcd; }"
        "QTabBar::scroller { background-color: transparent; }"
        "QTabBar::scroller QToolButton { background-color: #3b3b3b; color: #cdcdcd; border: none; padding: 0px; margin: 0px; min-width: 22px; min-height: 22px; border-radius: 0px; }";

    tw->setStyleSheet(style);
}
