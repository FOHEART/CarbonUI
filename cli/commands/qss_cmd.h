/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  qss_cmd.h
 * @brief qss 命令：导出样式函数实际写入控件的 QSS 文本。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#ifndef CARBONUI_CLI_QSS_CMD_H
#define CARBONUI_CLI_QSS_CMD_H

#include "cli_parser.h"

namespace carbonui {
namespace cli {

/**
 * @brief 导出指定控件（或全部控件）的 QSS（`qss --widget <type>` / `qss --all`）。
 *
 * @details
 * 实现方式是"真的创建一个控件并调用对应样式函数，再读回 styleSheet()"，
 * 因此输出与库的实际行为一致，不会出现"文档 QSS 与代码漂移"。
 * `--all` 时 `data` 为 `{ "<widget>": "<qss>", ... }`；单控件时
 * `data` 为 `{widget, qss}`。
 *
 * @param[in] args 去除全局旗标后的 token 列表。
 * @return 成功信封；未知控件类型返回用法错误（code 1）。
 */
Envelope runQss(const QStringList& args);

} // namespace cli
} // namespace carbonui

#endif // CARBONUI_CLI_QSS_CMD_H
