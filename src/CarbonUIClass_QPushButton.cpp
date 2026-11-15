/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QPushButton.cpp
 * @brief CarbonUIClass 中主色按钮样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::applyButtonStyle：按 ButtonShape 决定四角圆角，
 * 追加悬停 / 按下 / 禁用三态配色。公开接口的完整参数说明写在
 * CarbonUIClass.h；本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"


void CarbonUIClass::applyButtonStyle(QPushButton* btn, ButtonShape shape)
{
    if (!btn) return;

    // determine corner radii depending on shape
    int tl = 0, tr = 0, bl = 0, br = 0;
    switch (shape)
    {
        case Shape_RoundAll: tl = tr = bl = br = 10; break;
        case Shape_LeftRound: tl = bl = 10; tr = br = 0; break;
        case Shape_Rectangle: tl = tr = bl = br = 0; break;
        case Shape_RightRound: tr = br = 10; tl = bl = 0; break;
    }

    QString stylesheet = QString(
        "QPushButton { background-color: #009ee8; color: #ffffff; border: none; padding: 0px 2px; min-height: 28px; "
        "border-top-left-radius: %1px; border-bottom-left-radius: %2px; border-top-right-radius: %3px; border-bottom-right-radius: %4px; }")
        .arg(tl).arg(bl).arg(tr).arg(br);
    stylesheet += "QPushButton:hover { background-color: #00b0ff; }";
    stylesheet += "QPushButton:pressed { background-color: #007bb3; }";
    // 禁用态必须有独立规则，否则按钮被 setEnabled(false) 后仍渲染成可用态，
    // 用户分不清能不能点。灰底取自主题里既有的"非激活灰"，文字压暗但保持可读。
    stylesheet += "QPushButton:disabled { background-color: #636363; color: #cfcfcf; }";

    btn->setStyleSheet(stylesheet);
    btn->setCursor(Qt::PointingHandCursor);
}
