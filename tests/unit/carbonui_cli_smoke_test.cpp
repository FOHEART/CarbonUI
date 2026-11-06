/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  carbonui_cli_smoke_test.cpp
 * @brief carbonui_cli 的进程级冒烟：信封、退出码、产物文件、REPL 管道。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_tests
 *
 * @details
 * 用 QProcess 真启动 CLI 二进制（路径由 CMake 以 CARBONUI_CLI_EXE 注入），
 * 覆盖三类最容易回归的契约：
 * - 成功/失败路径的**进程退出码只分 0/1**，细分错误在信封 code；
 * - snapshot 必须真的落盘一张非空 PNG；
 * - REPL 吃带 UTF-8 BOM 的管道输入，`json` 开关对后续命令生效。
 */

#include <gtest/gtest.h>

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QStringList>

namespace {

/// @brief 一次 CLI 调用的观测结果。
struct CliRun
{
    int exitCode = -1;   ///< 进程退出码（无法启动时为负）。
    QString out;         ///< stdout（UTF-8 解码）。
    QString err;         ///< stderr（UTF-8 解码）。
    QJsonObject envelope; ///< 解析到的最后一个信封；解析失败时为空对象。
};

/// @brief 在 @p text 里找最后一个能解析成 JSON 对象的行（REPL 会输出多行）。
QJsonObject lastEnvelope(const QString& text)
{
    // 不用 split 的 SkipEmptyParts 标志：它在 Qt 5.14 起才有 Qt:: 版本，
    // 手写过滤兼容性最好。
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (int i = lines.size() - 1; i >= 0; --i)
    {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty())
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isObject())
            return doc.object();
    }
    return QJsonObject();
}

/// @brief 启动 CLI 并等它结束；@p stdinData 非空时从管道喂进去后关写端。
CliRun runCli(const QStringList& args, const QByteArray& stdinData = QByteArray())
{
    CliRun run;
    QProcess process;
    process.start(QStringLiteral(CARBONUI_CLI_EXE), args);
    if (!process.waitForStarted(15000))
    {
        run.exitCode = -100;
        return run;
    }
    if (!stdinData.isEmpty())
        process.write(stdinData);
    process.closeWriteChannel();
    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished(5000);
        run.exitCode = -101;
        return run;
    }
    run.exitCode = (process.exitStatus() == QProcess::NormalExit) ? process.exitCode() : -102;
    run.out = QString::fromUtf8(process.readAllStandardOutput());
    run.err = QString::fromUtf8(process.readAllStandardError());
    run.envelope = lastEnvelope(run.out);
    return run;
}

} // namespace

#ifndef CARBONUI_CLI_EXE

TEST(CliSmoke, Disabled)
{
    GTEST_SKIP() << "BUILD_CARBONUI_CLI=OFF：未注入 carbonui_cli 路径，跳过 CLI 冒烟";
}

#else

TEST(CliSmoke, InfoReturnsVersionAndEnvironment)
{
    const CliRun run = runCli(QStringList() << QStringLiteral("--json") << QStringLiteral("info"));

    ASSERT_EQ(0, run.exitCode) << run.err.toStdString();
    ASSERT_TRUE(run.envelope.value(QStringLiteral("ok")).toBool());
    const QJsonObject data = run.envelope.value(QStringLiteral("data")).toObject();
    EXPECT_FALSE(data.value(QStringLiteral("version")).toString().isEmpty());
    EXPECT_FALSE(data.value(QStringLiteral("qtVersion")).toString().isEmpty());
    // QPA 平台名必须存在：Win32 用原生 windows（offscreen 无字体数据库，
    // 整图画不出文字），其余平台 offscreen。两种都算合格。
    EXPECT_FALSE(data.value(QStringLiteral("qpa")).toString().isEmpty());
}

TEST(CliSmoke, SnapshotWritesNonEmptyPng)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString out = dir.filePath(QStringLiteral("button.png"));

    const CliRun run = runCli(QStringList()
                              << QStringLiteral("--json") << QStringLiteral("snapshot")
                              << QStringLiteral("--widget") << QStringLiteral("button")
                              << QStringLiteral("--shape") << QStringLiteral("rect")
                              << QStringLiteral("--out") << out);

    ASSERT_EQ(0, run.exitCode) << run.err.toStdString();
    ASSERT_TRUE(run.envelope.value(QStringLiteral("ok")).toBool());
    const QJsonObject data = run.envelope.value(QStringLiteral("data")).toObject();
    EXPECT_EQ(QStringLiteral("button"), data.value(QStringLiteral("widget")).toString());
    EXPECT_GT(data.value(QStringLiteral("bytes")).toDouble(), 0.0);
    EXPECT_TRUE(QFileInfo(out).exists()) << out.toStdString();
}

TEST(CliSmoke, GalleryWritesNonEmptyPng)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString out = dir.filePath(QStringLiteral("gallery.png"));

    const CliRun run = runCli(QStringList()
                              << QStringLiteral("--json") << QStringLiteral("gallery")
                              << QStringLiteral("--out") << out);

    ASSERT_EQ(0, run.exitCode) << run.err.toStdString();
    EXPECT_TRUE(run.envelope.value(QStringLiteral("ok")).toBool());
    EXPECT_GT(QFileInfo(out).size(), 0);
}

TEST(CliSmoke, QssAllCoversEveryWidget)
{
    const CliRun run = runCli(QStringList()
                              << QStringLiteral("--json") << QStringLiteral("qss")
                              << QStringLiteral("--all"));

    ASSERT_EQ(0, run.exitCode) << run.err.toStdString();
    const QJsonObject data = run.envelope.value(QStringLiteral("data")).toObject();
    EXPECT_TRUE(data.value(QStringLiteral("button")).toString().contains(QStringLiteral("QPushButton")))
        << data.value(QStringLiteral("button")).toString().toStdString();
    EXPECT_FALSE(data.value(QStringLiteral("checkbox")).toString().isEmpty());
    EXPECT_FALSE(data.value(QStringLiteral("groupbox")).toString().isEmpty());
}

TEST(CliSmoke, UsageErrorsExitOneAndCarryCodeOne)
{
    // 注意：不带 --widget 的 snapshot 是合法的（默认 button），所以"缺参数"用例
    // 用非法取值来表达。四条覆盖：未知命令、非法外形、未知控件、qss 缺 --widget。
    const QList<QStringList> badCalls = QList<QStringList>()
        << (QStringList() << QStringLiteral("--json") << QStringLiteral("bogus-command"))
        << (QStringList() << QStringLiteral("--json") << QStringLiteral("snapshot")
                          << QStringLiteral("--shape") << QStringLiteral("bogus"))
        << (QStringList() << QStringLiteral("--json") << QStringLiteral("snapshot")
                          << QStringLiteral("--widget") << QStringLiteral("nope"))
        << (QStringList() << QStringLiteral("--json") << QStringLiteral("qss"));

    for (int i = 0; i < badCalls.size(); ++i)
    {
        const CliRun run = runCli(badCalls.at(i));
        // 进程退出码只区分成败：失败恒为 1，细分错误在信封 code。
        EXPECT_EQ(1, run.exitCode) << "case " << i << " stderr=" << run.err.toStdString();
        EXPECT_FALSE(run.envelope.value(QStringLiteral("ok")).toBool()) << "case " << i;
        EXPECT_EQ(1, run.envelope.value(QStringLiteral("code")).toInt()) << "case " << i;
    }
}

TEST(CliSmoke, ReplAcceptsBomPipeAndJsonToggle)
{
    // 管道输入带 UTF-8 BOM 是脚本调用最常见的形态；json 开关必须真正影响后续命令。
    const QByteArray stdinData = QByteArray("\xEF\xBB\xBFjson\ninfo\nexit\n");

    const CliRun run = runCli(QStringList(), stdinData);

    ASSERT_EQ(0, run.exitCode) << run.err.toStdString();
    EXPECT_TRUE(run.out.contains(QStringLiteral("\"ok\":true"))) << run.out.toStdString();
    // 首条 json 命令即回一条状态信封：说明 BOM 已被剥掉、开关已生效。
    EXPECT_TRUE(run.out.contains(QStringLiteral("\"json\":true"))) << run.out.toStdString();
}

#endif // CARBONUI_CLI_EXE
