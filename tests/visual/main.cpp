/**
 * @file  main.cpp
 * @brief 渲染证据测试入口：默认切到平台原生后端，以便文字也能真实绘制。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui_tests
 *
 * @details
 * 与 `tests/unit/main.cpp` 的区别只有平台选择：
 *
 * - Windows 默认使用 `windows` 后端。`offscreen` 在该平台**没有字体库**
 *   （`QFontDatabase().families()` 返回空表），文字根本不会被绘制，
 *   像素级"文字是否画出来"的断言就失去意义；
 * - 其它平台保持 `offscreen`，并在字体库为空时由用例自行 `GTEST_SKIP()`。
 *
 * 本程序**从不调用 `show()`**，抓帧走 `QWidget::grab()`（离屏渲染到 QPixmap），
 * 因此不会弹出任何窗口，可在无人值守的机器上运行。
 *
 * 想强制指定后端时，预先设置环境变量即可（例如 `QT_QPA_PLATFORM=offscreen`）。
 *
 * 线程约定：与所有 Qt Widgets 用例一样，只在主线程执行。
 */

#include <gtest/gtest.h>

#include <QtCore/QByteArray>
#include <QtCore/QtGlobal>
#include <QtWidgets/QApplication>

/**
 * @brief 渲染证据测试入口：先初始化 gtest，切到可绘制字体的后端，再建 QApplication。
 *
 * @param[in] argc 命令行参数个数，由运行器提供，取值 [1, +∞)。
 * @param[in] argv 命令行参数数组，不可为 NULL；`--gtest_*` 会先被
 *                 `::testing::InitGoogleTest` 摘除，剩余参数交给 QApplication。
 * @return 用例执行结果，0 表示全部通过，非 0 表示存在失败用例。
 */
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    {
#if defined(Q_OS_WIN)
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows"));
#else
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
#endif
    }

    QApplication app(argc, argv);
    Q_UNUSED(app);

    return RUN_ALL_TESTS();
}
