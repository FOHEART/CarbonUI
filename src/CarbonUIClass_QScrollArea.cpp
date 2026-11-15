/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QScrollArea.cpp
 * @brief CarbonUIClass 中无边框滚动区样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::applyScrollAreaStyle：无边框、收窄到 4px 的
 * 纵向滚动条。公开接口的完整参数说明写在 CarbonUIClass.h；本文件只补充
 * 实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtWidgets/QScrollBar>


void CarbonUIClass::applyScrollAreaStyle(QScrollArea* area)
{
    if (!area) return;

    area->setStyleSheet(
        "QScrollArea { border: none; }"
        "QScrollArea QWidget { background-color: #3b3b3b; }"
        "QScrollArea QScrollBar:vertical { width: 4px; background: transparent; margin: 0px; }"
        "QScrollArea QScrollBar::handle:vertical { background: #cdcdcd; min-height: 20px; border-radius: 4px; }"
        "QScrollArea QScrollBar::add-line:vertical, QScrollArea QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollArea QScrollBar::add-page:vertical, QScrollArea QScrollBar::sub-page:vertical { background: none; }"
    );

    // ensure scrollbars have minimal width
    if (auto* v = area->verticalScrollBar())
    {
        v->setMinimumWidth(4);
    }
}
