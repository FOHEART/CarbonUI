/**
 * @file  main.cpp
 * @brief GoogleTest 入口：先初始化 gtest，再创建 QApplication。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 顺序不能颠倒：`InitGoogleTest` 必须先摘掉 `--gtest_*` 参数，否则
 * QApplication 会把它们当成 Qt 参数解析（Qt 5 会直接报 unknown option）。
 * 本可执行文件自带 `main`，因此只链接 `GTest::gtest`，不链接 `gtest_main`，
 * 避免 `LNK2005: main already defined`。
 *
 * 线程约定：Qt Widgets 只允许在主线程访问，所有用例都在此 main 所在线程执行。
 */

#include <gtest/gtest.h>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>

/**
 * @brief 测试可执行文件入口：先初始化 GoogleTest，再创建 QApplication，最后运行全部用例。
 *
 * @param[in] argc 命令行参数个数，由运行器提供，取值 [1, +∞)。
 * @param[in] argv 命令行参数数组，不可为 NULL；`--gtest_*` 参数会先被
 *                 `testing::InitGoogleTest` 摘除，剩余参数原样交给 QApplication。
 * @return 用例执行结果，0 表示全部通过，非 0 表示存在失败用例（取值来自 RUN_ALL_TESTS()）。
 */
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // Suites create real widgets but must also run on CI without a display.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    }

    QApplication app(argc, argv);
    Q_UNUSED(app);

    return RUN_ALL_TESTS();
}
