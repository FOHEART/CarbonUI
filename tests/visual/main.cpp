/**
 * @file  main.cpp
 * @brief 渲染证据测试入口：默认切到平台原生后端，以便文字也能真实绘制。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
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
 * 截图基线还要求"文字字形可复现"：Qt 默认使用系统 UI 字体，中文 Windows 上是
 * Microsoft YaHei UI、CI 英文镜像是 Segoe UI，同一句话的像素完全不同，基线跨机器
 * 永远对不上。因此入口显式固定字体族与字号（见 `pinReproducibleFont()`）。
 *
 * 线程约定：与所有 Qt Widgets 用例一样，只在主线程执行。
 */

#include <gtest/gtest.h>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtGlobal>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QApplication>

namespace
{
/**
 * @brief 固定 QApplication 的默认字体族与字号，让文字渲染跨机器可复现。
 *
 * @details
 * 按平台给出首选/备选字体族（Windows：Segoe UI；其它：DejaVu Sans），从
 * `QFontDatabase().families()` 里挑第一个实际存在的；一个都没有时保持系统默认，
 * 此时基线差异只能靠容差吸收。字号固定为 9pt，避免不同 DPI 设置下的度量差异。
 *
 * @param[in] app 已构造的 QApplication，不可为 NULL。
 */
void pinReproducibleFont(QApplication* app)
{
#if defined(Q_OS_WIN)
    const QStringList candidates = QStringList() << QStringLiteral("Segoe UI")
                                                 << QStringLiteral("Tahoma")
                                                 << QStringLiteral("Arial");
#else
    const QStringList candidates = QStringList() << QStringLiteral("DejaVu Sans")
                                                 << QStringLiteral("Liberation Sans")
                                                 << QStringLiteral("Noto Sans");
#endif

    const QStringList available = QFontDatabase().families();
    for (const QString& family : candidates)
    {
        if (available.contains(family, Qt::CaseInsensitive))
        {
            QFont font = app->font();
            font.setFamily(family);
            font.setPointSize(9);
            app->setFont(font);
            return;
        }
    }
}
} // namespace

/**
 * @brief 渲染证据测试入口：先初始化 gtest，切到可绘制字体的后端，再建 QApplication。
 *
 * @param[in] argc 命令行参数个数，由运行器提供，取值 [1, +∞)。
 * @param[in] argv 命令行参数数组，不可为 NULL；`--gtest_*` 会先被
 *                 `testing::InitGoogleTest` 摘除，剩余参数交给 QApplication。
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

    // 截图基线要求同平台内像素可复现：固定缩放因子与逻辑 DPI，否则 150% 缩放的
    // 开发机会抓出比 CI 大 1.5 倍的图，尺寸一变基线永远对不上。必须在 QApplication
    // 构造之前设置，且尊重调用方已经显式指定的值。
    if (qEnvironmentVariableIsEmpty("QT_SCALE_FACTOR"))
        qputenv("QT_SCALE_FACTOR", QByteArrayLiteral("1"));
    if (qEnvironmentVariableIsEmpty("QT_AUTO_SCREEN_SCALE_FACTOR"))
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", QByteArrayLiteral("0"));
    if (qEnvironmentVariableIsEmpty("QT_FONT_DPI"))
        qputenv("QT_FONT_DPI", QByteArrayLiteral("96"));

    QApplication app(argc, argv);
    pinReproducibleFont(&app);

    return RUN_ALL_TESTS();
}
