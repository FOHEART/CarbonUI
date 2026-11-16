/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QStatusBar.cpp
 * @brief CarbonUIClass 中状态栏样式助手的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 实现 @ref CarbonUIClass::setupQStatusBarStyle ——深色底 + 次级文字色。
 * 公开接口的完整参数说明写在 CarbonUIClass.h；本文件只补充实现细节。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"


void CarbonUIClass::setupQStatusBarStyle(QStatusBar* sb)
{
    if (!sb)
        return;
   sb->setStyleSheet("background-color: #2e2e2e; color: #676767; border: none; outline: none;");
}
