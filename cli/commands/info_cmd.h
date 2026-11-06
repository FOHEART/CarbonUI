/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  info_cmd.h
 * @brief info 命令：报告库版本与渲染环境，供 Agent 判定能否出图。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#ifndef CARBONUI_CLI_INFO_CMD_H
#define CARBONUI_CLI_INFO_CMD_H

#include "cli_parser.h"

namespace carbonui {
namespace cli {

/**
 * @brief 输出库版本、Qt 版本、平台与默认字体族（`info`）。
 *
 * @details
 * `data` 字段：`version`/`versionTag`/`qtVersion`/`platform`/`qpa`/
 * `defaultFontFamily`/`fontFamilies`（前 5 个候选，便于判断字体引擎是否可用）。
 * 字体缺失不会让本命令失败——出图质量由调用方结合 `snapshot` 自行判断。
 *
 * @param[in] args 去除全局旗标后的 token 列表（本命令不接收选项）。
 * @return 成功信封。
 */
Envelope runInfo(const QStringList& args);

} // namespace cli
} // namespace carbonui

#endif // CARBONUI_CLI_INFO_CMD_H
