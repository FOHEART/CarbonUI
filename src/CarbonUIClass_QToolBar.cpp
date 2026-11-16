/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QToolBar.cpp
 * @brief CarbonUIClass 中工具栏样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::setupToolBarStyle ——禁止拖动/浮动、固定 32px
 * 上限高、28px 图标与深底配色。公开接口的完整参数说明写在
 * CarbonUIClass.h；本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtCore/QSize>
#include <QtWidgets/QLayout>


void CarbonUIClass::setupToolBarStyle(QToolBar* tb)
{
    if (!tb) return;

    // set toolbar icon size and add down icon
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setFixedHeight(32);
 tb->setIconSize(QSize(28, 28));

    int iconWidth = tb->iconSize().width();
    //int spacing = iconWidth / 4; // horizontal spacing = half icon width
    int spacing = 2;
    QString tbStyle = QString(
        "QToolBar { background: #3b3b3b;  border-top: 1px solid #212121; }"
        "QToolBar::handle { width: 0px; }"
        "QToolButton { border: none; border-radius: 10px; padding: 2px; }"
        "QToolButton:hover { border: 1px solid #5d5d5d; }"
    );

    tb->setStyleSheet(tbStyle);

    if (tb->layout())
    {
        tb->layout()->setSpacing(spacing);
    }

    tb->setContentsMargins(32, 0, spacing / 2, 0);

  
}
