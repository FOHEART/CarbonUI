/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @defgroup carbonui_cli carbonui_cli（命令行工具）
 * @brief 独立薄 CLI：信封协议、渲染命令与控件工厂。
 */

/**
 * @file  cli_parser.h
 * @brief carbonui_cli 的公共工具：统一信封、选项解析、枚举映射。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 *
 * @details
 * CLI 输出协议贯穿所有命令：`{"ok":bool,"code":int,"message":str,"data":{}}`，
 * 见根目录 SKILL.md 的错误码表。这里刻意不使用 QCommandLineParser：
 * 它有"首元素当程序名吞掉""出错即 process() 退出进程"两类行为，与 REPL
 * 复用解析直接冲突，手写解析反而最短。退出码约定：进程只分 0（成功）与
 * 1（失败），细分错误一律放信封的 code 字段。
 */

#ifndef CARBONUI_CLI_PARSER_H
#define CARBONUI_CLI_PARSER_H

#include <QString>
#include <QStringList>
#include <QJsonObject>

#include "CarbonUIClass.h"

namespace carbonui {
namespace cli {

/// @brief CLI 错误码表（信封 code 字段；进程退出码只取 ok 布尔）。
enum ExitCode
{
    CodeOk      = 0, ///< 成功。
    CodeUsage   = 1, ///< 参数/用法错误：未知命令、未知取值、缺必填项（Agent 应改命令重试）。
    CodeIo      = 2, ///< 输出文件无法写入（目录不存在、无权限、路径被占用）。
    CodeRender  = 3  ///< 渲染失败：抓到的图像为空（QPA 环境异常等）。
};

/**
 * @brief 统一 JSON 信封。
 *
 * @details
 * @p data 永远存在（失败时为空对象）；字段命名跨命令保持一致。
 */
struct Envelope
{
    bool ok;             ///< 命令是否成功。
    int code;            ///< 细分错误码，取值见 `ExitCode` 枚举。
    QString message;     ///< 人类可读的一句话结果或错误说明。
    QJsonObject data;    ///< 命令输出数据，失败时为空对象。
};

/**
 * @brief 构造成功信封，@p data 原样放入（可为空对象）。
 *
 * @param data 输出数据对象，可为空对象。
 * @return 成功信封（ok=true、code=CodeOk）。
 */
Envelope makeOk(const QJsonObject& data = QJsonObject());

/**
 * @brief 构造失败信封，@p code 取值见 `ExitCode` 枚举。
 *
 * @param code    细分错误码。
 * @param message 人类可读的一句话错误说明。
 * @return 失败信封（ok=false、data 为空对象）。
 */
Envelope makeError(int code, const QString& message);

/**
 * @brief 序列化为紧凑 JSON 文本（无多余空白，便于 subprocess 逐行解析）。
 *
 * @param[in] env 待序列化的信封。
 * @return 单行紧凑 JSON 字符串。
 */
QString envelopeJson(const Envelope& env);

/**
 * @brief 按当前模式打印信封并换行。
 *
 * @param[in] env  待输出的信封。
 * @param[in] json true 输出单行紧凑 JSON；false 输出多行人类可读文本。
 */
void printEnvelope(const Envelope& env, bool json);

/**
 * @brief 把一行输入切成 token：剥离 UTF-8 BOM，支持成对单/双引号（路径含空格）。
 *
 * @param[in] line 原始输入行（可含 BOM）。
 * @return token 列表，引号已被剥掉；空行返回空列表。
 */
QStringList tokenizeLine(const QString& line);

/**
 * @brief 判断是否带了布尔旗标（如 `--json`）。
 *
 * @param[in] args token 列表。
 * @param[in] flag 旗标名（含 `--` 前缀）。
 * @return 存在返回 true，否则 false。
 */
bool hasFlag(const QStringList& args, const QString& flag);

/**
 * @brief 取 `--name value` 形式的选项值。
 *
 * @param[in]  args  token 列表。
 * @param[in]  name  选项名（含 `--` 前缀）。
 * @param[out] found 未出现时写 false；出现但缺值视为用法错误，由调用方报错。
 * @return 选项值；未出现或缺值时返回空串。
 */
QString optionValue(const QStringList& args, const QString& name, bool* found);

/// @brief snapshot/qss 支持的控件类型清单（含 --all 时的展开顺序）。
/// @return 类型名列表，顺序固定。
QStringList widgetTypes();

/**
 * @brief 把 `round|left|right|rect` 映射为按钮外形。
 *
 * @param[in]  text  外形字符串；空串按 round 处理。
 * @param[out] shape 映射结果；非法取值时不写。
 * @return 合法返回 true，否则 false。
 */
bool parseButtonShape(const QString& text, CarbonUIClass::ButtonShape* shape);

/**
 * @brief 把 `round|left|right|rect` 映射为单选按钮外形。
 *
 * @param[in]  text  外形字符串；空串按 round 处理。
 * @param[out] shape 映射结果；非法取值时不写。
 * @return 合法返回 true，否则 false。
 */
bool parseRadioShape(const QString& text, CarbonUIClass::RadioShape* shape);

} // namespace cli
} // namespace carbonui

#endif // CARBONUI_CLI_PARSER_H
