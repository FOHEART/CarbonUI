/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIWindowButtonStyle.h
 * @brief 无边框窗口标题条「窗口按钮」外观的唯一实现处（库内部头，不对外安装）。
 * @author CarbonUI
 * @date  2026-11-24
 * @ingroup carbonui
 *
 * @details
 * CarbonUIDialog（46px 对话框标题条）与 CarbonUIQTitleBar（26px 主窗标题条）
 * 共用同一套窗口按钮 QSS 与标题条底色：透明底白字、悬停淡灰底圆角。标题条
 * 底色 #0f0f0f 很暗，悬停底色必须明显亮于底色才能一眼看出反馈（历史坑：旧的
 * #2a2a2a 只比底色亮一点；主窗标题条若没开 WA_StyledBackground，透出的宿主
 * #212121 与 #2a2a2a 仅差 9 级灰，几乎看不出变化），因此定为 #3a3a3a。
 * 改配色只改这里，两个控件自动保持一致。
 *
 * 仅依赖 QtCore 的 QString，可被库内任意 TU 包含；随 .cpp 一起编译，不进
 * 安装头目录。
 */

#ifndef CARBONUIWINDOWBUTTONSTYLE_H
#define CARBONUIWINDOWBUTTONSTYLE_H

#include <QtCore/QString>

namespace CarbonUIWindowChrome {

/**
 * @brief 标题条窗口按钮 QSS：透明底白字，悬停明显淡灰底圆角。
 * @param[in] objectName 按钮的 objectName（QSS 按 id 选择器限定作用范围）。
 */
inline QString windowButtonStyle(const QString& objectName)
{
    return QStringLiteral("QPushButton#%1 { background-color: transparent; color: #ffffff;"
                          " border: none; padding: 0px; }"
                          "QPushButton#%1:hover { background-color: #3a3a3a;"
                          " border-radius: 6px; }")
        .arg(objectName);
}

/**
 * @brief 标题条 QSS：纯黑底 + 上两角圆角（下两角保持直角以贴合正文）。
 * @param[in] objectName 标题条控件的 objectName。
 * @param[in] radius     上两角圆角半径；最大化时传 0。
 */
inline QString headerStyle(const QString& objectName, int radius)
{
    return QStringLiteral("QWidget#%1 { background-color: #0f0f0f;"
                          " border-top-left-radius: %2px; border-top-right-radius: %2px; }")
        .arg(objectName)
        .arg(radius);
}

} // namespace CarbonUIWindowChrome

#endif // CARBONUIWINDOWBUTTONSTYLE_H
