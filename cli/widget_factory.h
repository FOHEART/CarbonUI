/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  widget_factory.h
 * @brief 按类型创建"已套样式"的控件，供 snapshot 与 qss 两条命令共用。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 *
 * @details
 * 工厂只负责"建控件 + 调一次库的样式函数 + 给一个默认尺寸"，不负责抓图。
 * 这样 qss 命令可以只读 `styleSheet()` 而不渲染，snapshot 再额外决定宿主
 * 底色与边距。任何控件都**不 show()**，几何由调用方用 resize() 决定。
 */

#ifndef CARBONUI_CLI_WIDGET_FACTORY_H
#define CARBONUI_CLI_WIDGET_FACTORY_H

#include <QString>
#include <QWidget>

namespace carbonui {
namespace cli {

/// @brief snapshot / qss 的控件构造参数。
struct WidgetSpec
{
    QString type;   ///< 控件类型，取值见 widgetTypes()。
    QString shape;  ///< 外形：round|left|right|rect（仅 button/radio 生效）。
    QString state;  ///< 状态：normal|disabled|checked|collapsed。
    QString text;   ///< 控件文字；空串时用该类型的默认文案。
};

/**
 * @brief 创建一个已套用 CarbonUI 样式的控件（无父对象，调用方负责释放）。
 *
 * @param[in]  spec   构造参数；@p type 必须先经 widgetTypes() 校验。
 * @param[out] error  失败原因（如外形取值非法），成功时为空。
 * @return 新控件；失败返回 NULL。
 */
QWidget* createStyledWidget(const WidgetSpec& spec, QString* error);

} // namespace cli
} // namespace carbonui

#endif // CARBONUI_CLI_WIDGET_FACTORY_H
