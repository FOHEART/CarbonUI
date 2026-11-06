/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  main.cpp
 * @brief carbonui_cli 入口：命令分发 + REPL。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 *
 * @details
 * 形态是**独立薄 CLI 可执行文件**（库工程没有 GUI 主程序可供 `--cli` 分流）。
 * 三条铁律：
 * - 不 show() 任何窗口；QPA 缺省 offscreen，抓图靠 QWidget::grab()，
 *   因此可在无头 CI 里运行（与 tests/visual 的截图基线同一套机制）；
 * - 每条命令一个进程，全部状态走参数，不写会话文件（本库无"当前项目"概念）；
 * - 进程退出码只分 0/1，细分错误放信封 code（见 cli_parser.h 的错误码表）。
 *
 * REPL：无参数启动进入；`json` 切换后续输出模式，`exit`/`quit`/EOF 退出。
 * 这里刻意不用 QCommandLineParser：它会"出错即退出进程"，一条错误命令就会
 * 杀掉整个 REPL，且首元素会被当作程序名吞掉。
 */

#include <QApplication>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

#include "cli_parser.h"
#include "commands/info_cmd.h"
#include "commands/qss_cmd.h"
#include "commands/render_cmd.h"

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif

using carbonui::cli::Envelope;
using carbonui::cli::makeError;
using carbonui::cli::makeOk;

namespace {

/// @brief 标准输入是否连在终端上（决定 REPL 是否打印提示符，避免污染管道输出）。
bool stdinIsTerminal()
{
#ifdef Q_OS_WIN
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

/// @brief 帮助信封：命令表放 data，文本模式直接多行 JSON。
Envelope helpEnvelope()
{
    QJsonArray commands;
    const char* names[] = {"info", "snapshot", "gallery", "qss", "help", "version"};
    const char* summaries[] = {
        "库版本 / Qt 版本 / QPA / 默认字体族",
        "渲染单个控件到 PNG（--widget/--shape/--state/--text/--width/--height/--out）",
        "渲染整套控件总览到 PNG（--out）",
        "导出控件 QSS（--widget 或 --all）",
        "本帮助",
        "同 info"};
    for (int i = 0; i < 6; ++i)
    {
        QJsonObject entry;
        entry.insert(QStringLiteral("name"), QString::fromLatin1(names[i]));
        entry.insert(QStringLiteral("summary"), QString::fromUtf8(summaries[i]));
        commands.append(entry);
    }

    QJsonObject data;
    data.insert(QStringLiteral("usage"),
                QStringLiteral("carbonui_cli [--json] <command> [options]  或  无参数进入 REPL"));
    data.insert(QStringLiteral("commands"), commands);
    Envelope env = makeOk(data);
    env.message = QStringLiteral("carbonui_cli 命令表");
    return env;
}

/// @brief 执行一次命令（@p tokens 首元素是命令名，其余是选项）。
Envelope execute(const QStringList& tokens)
{
    if (tokens.isEmpty())
        return makeError(carbonui::cli::CodeUsage, QStringLiteral("缺少命令"));

    const QString command = tokens.first();
    const QStringList args = tokens.mid(1);

    if (command == QLatin1String("info") || command == QLatin1String("version"))
        return carbonui::cli::runInfo(args);
    if (command == QLatin1String("snapshot"))
        return carbonui::cli::runSnapshot(args);
    if (command == QLatin1String("gallery"))
        return carbonui::cli::runGallery(args);
    if (command == QLatin1String("qss"))
        return carbonui::cli::runQss(args);
    if (command == QLatin1String("help"))
        return helpEnvelope();

    return makeError(carbonui::cli::CodeUsage,
                     QStringLiteral("未知命令 %1（可用：info snapshot gallery qss help version）")
                         .arg(command));
}

/**
 * @brief REPL 主循环：逐行读、逐行输出信封。
 *
 * @param[in,out] jsonMode 初始输出模式；`json` 行会就地翻转它。
 * @return 进程退出码（REPL 正常结束恒为 0）。
 */
int runRepl(bool jsonMode)
{
    QTextStream in(stdin);
    in.setCodec("UTF-8");
    QTextStream prompt(stdout);
    prompt.setCodec("UTF-8");
    const bool interactive = stdinIsTerminal();

    if (interactive)
        prompt << "carbonui_cli REPL（help 看命令，json 切换输出格式，exit 退出）\n" << Qt::flush;

    for (;;)
    {
        if (interactive)
            prompt << "carbonui> " << Qt::flush;
        const QString line = in.readLine();
        if (line.isNull())
        {
            if (interactive)
                prompt << "\n";
            break; // EOF（Ctrl+D / Ctrl+Z）
        }

        // tokenizeLine 内部会剥掉脚本管道常带的 UTF-8 BOM。
        QStringList tokens = carbonui::cli::tokenizeLine(line);
        if (tokens.isEmpty())
            continue;

        const QString command = tokens.first();
        if (command == QLatin1String("exit") || command == QLatin1String("quit"))
            break;
        if (command == QLatin1String("json"))
        {
            // 切换后立刻回一条当前状态，避免"只打印 on/off 却没真正生效"的常见错觉。
            jsonMode = !jsonMode;
            QJsonObject state;
            state.insert(QStringLiteral("json"), jsonMode);
            carbonui::cli::printEnvelope(makeOk(state), jsonMode);
            continue;
        }

        tokens.removeAll(QStringLiteral("--json"));
        carbonui::cli::printEnvelope(execute(tokens), jsonMode);
    }
    return 0;
}

} // namespace

/**
 * @brief CLI 进程入口：单命令模式或 REPL。
 *
 * @param argc 参数个数（含程序名）。
 * @param argv 参数数组；首个非程序名 token 是命令。
 * @return 0 成功 / 1 失败（REPL 正常结束恒为 0）。
 */
int main(int argc, char** argv)
{
#ifdef Q_OS_WIN
    // 控制台子系统本就可用，只需切 UTF-8 代码页让中文正常显示。
    SetConsoleOutputCP(CP_UTF8);
#endif

    // 抓图从不 show() 窗口，但**需要字体数据库**：Windows 的 offscreen 插件
    // 没有 font database，整图会一个字都画不出来（实测踩过）。所以平台策略
    // 与 tests/visual 相同：Win32 用原生 windows QPA（CI 的 windows runner
    // 有桌面会话），其余用 offscreen。尊重调用方已显式指定的值。
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    {
#if defined(Q_OS_WIN)
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows"));
#else
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
#endif
    }

    QApplication app(argc, argv);

#ifdef CARBONUI_CLI_BUNDLED_FONT
    // 仓库内置字体：本地与 CI 出同一字形，渲染结果跨机器可复现；
    // 安装态没有该文件时静默回退系统字体。
    const QString bundledFont = QStringLiteral(CARBONUI_CLI_BUNDLED_FONT);
    if (QFileInfo::exists(bundledFont))
    {
        const int fontId = QFontDatabase::addApplicationFont(bundledFont);
        if (fontId >= 0)
        {
            const QStringList families = QFontDatabase().applicationFontFamilies(fontId);
            if (!families.isEmpty())
                QGuiApplication::setFont(QFont(families.first()));
        }
    }
#endif

    QStringList tokens;
    for (int i = 1; i < argc; ++i)
        tokens << QString::fromLocal8Bit(argv[i]);

    bool jsonMode = tokens.removeAll(QStringLiteral("--json")) > 0;
    if (tokens.isEmpty())
        return runRepl(jsonMode);

    const Envelope env = execute(tokens);
    carbonui::cli::printEnvelope(env, jsonMode);
    return env.ok ? 0 : 1;
}
