/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  render_cmd.h
 * @brief snapshot / gallery 两条渲染命令：把控件样式渲染成 PNG。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#ifndef CARBONUI_CLI_RENDER_CMD_H
#define CARBONUI_CLI_RENDER_CMD_H

#include "cli_parser.h"

namespace carbonui {
namespace cli {

/**
 * @brief 渲染单个控件到 PNG（`snapshot --widget <type> [--shape s] [--state st]`）。
 *
 * @details
 * 控件建在一块 8px 边距、panel 底色 \#272727 的宿主上，调用相应
 * `CarbonUIQ*::applyStyle` 后 `QWidget::grab()` 抓帧，全程不 `show()` 任何
 * 窗口（QPA 固定 offscreen），可在 CI 无头环境运行。
 *
 * 支持的 `--widget`：`button | radio | checkbox | lineedit | tab | combo |
 * tree | statusbar | groupbox`（与 cli_parser.h 的 widgetTypes() 一致）。
 * `--state` 目前识别
 * `normal|disabled|checked|collapsed`（不同控件含义见 SKILL.md）。
 *
 * @param[in] args 去除全局旗标后的 token 列表。
 * @return 成功信封；`data` 含 `widget/out/width/height/bytes`。
 */
Envelope runSnapshot(const QStringList& args);

/**
 * @brief 渲染整套控件总览到 PNG（`gallery --out <png>`）。
 *
 * @details
 * 版面与视觉回归基线 `form-overview.png` 同源：标题、主按钮、三段单选条、
 * 指示灯复选框 + 次级色标签、数字输入框、下拉框。用于快速人工/Agent 审阅
 * 当前主题整体观感。
 *
 * @param[in] args 去除全局旗标后的 token 列表。
 * @return 成功时 `data` 含 `out/width/height/bytes`。
 */
Envelope runGallery(const QStringList& args);

} // namespace cli
} // namespace carbonui

#endif // CARBONUI_CLI_RENDER_CMD_H
