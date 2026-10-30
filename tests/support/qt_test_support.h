/**
 * @file  qt_test_support.h
 * @brief 单元测试共用的 Qt/GoogleTest 辅助断言与事件泵。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui_tests
 *
 * @details
 * 本文件只提供头文件实现，无全局状态，供 `tests/unit` 下所有用例包含。
 * 线程约定：所有辅助函数都必须在测试主线程调用（Qt Widgets 非线程安全）。
 */

#ifndef CARBONUI_TESTS_QT_TEST_SUPPORT_H
#define CARBONUI_TESTS_QT_TEST_SUPPORT_H

#include <gtest/gtest.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QFontDatabase>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QLayout>
#include <QtWidgets/QWidget>

#include <functional>

/**
 * @defgroup carbonui_tests CarbonUI 单元测试
 * @brief GoogleTest 用例与测试辅助断言，验证 CarbonUI 的对外行为。
 *
 * @details
 * 用例通过真实的 Qt 控件驱动公开接口，只断言调用方可观测的状态（几何、样式表、
 * 子控件、动画结束后的可见性），不复制实现细节；所有用例都能在
 * `QT_QPA_PLATFORM=offscreen` 下无显示环境运行。
 */

namespace carbonui_test
{

/**
 * @brief 判断样式表文本是否包含某段 QSS 片段。
 *
 * @param[in] styleSheet 待检查的样式表，可为空字符串。
 * @param[in] needle     期望出现的片段，不可为 NULL，不能为空字符串。
 * @return 断言结果；失败时打印完整样式表，便于定位是哪条规则缺失。
 * @retval AssertionSuccess 片段存在。
 * @retval AssertionFailure 片段缺失，或 @p needle 为空。
 */
inline ::testing::AssertionResult qssContains(const QString& styleSheet, const char* needle)
{
    if (!needle || !*needle)
        return ::testing::AssertionFailure() << "needle must be a non-empty string";

    if (styleSheet.contains(QLatin1String(needle)))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure()
           << "QSS does not contain \"" << needle << "\", actual QSS:\n"
           << styleSheet.toStdString();
}

/**
 * @brief 统计子串出现次数，用于验证"样式只追加一次/被重复追加"这类行为。
 *
 * @param[in] haystack 被搜索的文本。
 * @param[in] needle   要计数的片段，不可为 NULL；为空时返回 0。
 * @return 出现次数，取值 [0, +∞)；@p needle 为空时为 0。
 */
inline int countOccurrences(const QString& haystack, const char* needle)
{
    if (!needle || !*needle)
        return 0;

    const QString pattern = QLatin1String(needle);
    int count = 0;
    int from = 0;
    while (true)
    {
        const int at = haystack.indexOf(pattern, from);
        if (at < 0)
            break;
        ++count;
        from = at + pattern.size();
    }
    return count;
}

/**
 * @brief 在事件循环中反复处理事件，直到条件成立或超时。
 *
 * @details
 * 用于等待 `QPropertyAnimation` 这类由事件驱动完成的状态迁移，替代固定的
 * `sleep`：机器快时不空等，机器慢时也不会误判。
 *
 * @param[in] predicate 每次处理完一批事件后求值的条件，不可为空。
 * @param[in] timeoutMs 超时时间，单位毫秒，取值 [1, 60000]，默认 3000。
 * @return 超时后 @p predicate 的最终取值；超时仍未成立时返回 false。
 */
inline bool spinUntil(const std::function<bool()>& predicate, int timeoutMs = 3000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    return predicate();
}

/**
 * @brief 显示控件并处理一轮事件，使焦点/几何信息可用。
 *
 * @param[in,out] widget 待激活的顶层控件，可为 NULL；为 NULL 时不做任何事。
 */
inline void showAndSettle(QWidget* widget)
{
    if (!widget)
        return;

    widget->show();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

/**
 * @brief 把控件离屏渲染成图片，并把 PNG 落到 `ui-artifacts/` 供人工查看。
 *
 * @details
 * 走的是真实绘制管线（`QWidget::grab()` → QPainter → 光栅引擎），因此样式表、
 * 圆角、子控件布局、字体都真正参与绘制；输出目标是内存里的 QImage，**不调用
 * `show()`**，所以不会弹出任何窗口。
 *
 * 注意两点取景要求：
 * - 要验证"圆角"必须渲染**父容器**：控件圆角之外是透明的，只有父容器的底色
 *   能证明那个角确实被切掉了；单独 grab 一个有背景色的控件，四角也是背景色。
 * - 文字绘制依赖平台字体库。`QT_QPA_PLATFORM=offscreen` 在 Windows 上字体库为空
 *   （`QFontDatabase().families()` 为 0），此时只剩填充色，没有文字与抗锯齿。
 *
 * PNG 文件名形如 `01-button.png`，保存在测试可执行文件同级的 `ui-artifacts/`
 * 目录下（目录不存在会自动创建）。
 *
 * @param[in,out] widget   待渲染的控件（通常是装了被测控件的父容器），
 *                         **不可为 NULL**；未显式 resize 过时按 `sizeHint()` 定尺寸。
 * @param[in]     fileName PNG 文件名，不可为空字符串，建议以序号开头便于排序。
 * @return 渲染结果，格式固定为 `QImage::Format_ARGB32`；保存失败时仍返回图像。
 * @note 仅在 GUI 主线程调用。
 */
inline QImage renderToArtifact(QWidget* widget, const QString& fileName)
{
    if (!widget)
        return QImage();

    // 调用方显式 resize() 过的控件（Qt 会置 WA_Resized）保持原尺寸，
    // 否则按 sizeHint 收缩到自然大小。
    if (!widget->testAttribute(Qt::WA_Resized))
        widget->adjustSize();

    widget->ensurePolished();
    if (QLayout* layout = widget->layout())
        layout->activate();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 30);

    const QImage image = widget->grab().toImage().convertToFormat(QImage::Format_ARGB32);

    QDir dir(QCoreApplication::applicationDirPath() + QStringLiteral("/ui-artifacts"));
    if (dir.exists() || QDir().mkpath(dir.absolutePath()))
        image.save(dir.absoluteFilePath(fileName));

    return image;
}

/**
 * @brief 判断当前平台能否真正绘制文字。
 *
 * @details
 * 只取一个字体族名做探测：字体库为空时 `QFontDatabase().families()` 返回空表，
 * 此时 Qt 画不出任何字形，像素断言必须跳过文字相关项。
 *
 * @return true 表示至少有 1 个可用字体族，可以断言文字像素；
 *         false 表示字体库为空（典型是 offscreen 平台），应跳过文字断言。
 */
inline bool fontsAvailable()
{
    static const bool available = !QFontDatabase().families().isEmpty();
    return available;
}

/**
 * @brief 取指定像素的颜色名，供 gtest 打印可读的期望/实际值。
 *
 * @param[in] image 已渲染的图像，可为 NULL 图像。
 * @param[in] x     横坐标，单位像素，取值 [0, image.width()-1]；越界时返回 "out-of-range"。
 * @param[in] y     纵坐标，单位像素，取值 [0, image.height()-1]；越界时返回 "out-of-range"。
 * @return 形如 `#009ee8` 的颜色名；越界时返回字面量 "out-of-range"。
 */
inline QString pixelName(const QImage& image, int x, int y)
{
    if (image.isNull() || x < 0 || y < 0 || x >= image.width() || y >= image.height())
        return QStringLiteral("out-of-range");

    return image.pixelColor(x, y).name(QColor::HexRgb);
}

/**
 * @brief 统计图像中不同颜色的数量，用于判断"是否真的画上了东西"。
 *
 * @param[in] image 已渲染的图像，可为 NULL 图像。
 * @return 不同颜色个数；空图像返回 0。单一纯色填充返回 1。
 */
inline int distinctColourCount(const QImage& image)
{
    if (image.isNull())
        return 0;

    QSet<QRgb> colours;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            colours.insert(image.pixel(x, y));

    return colours.size();
}

} // namespace carbonui_test

/**
 * @brief 断言样式表包含指定 QSS 片段，失败时输出完整样式表。
 *
 * @param[in] style  被检查的样式表（QString）。
 * @param[in] needle 期望出现的 QSS 片段（C 字符串）。
 */
#define EXPECT_QSS_CONTAINS(style, needle) \
    EXPECT_TRUE(carbonui_test::qssContains((style), (needle)))

/**
 * @brief 断言样式表不包含指定 QSS 片段，失败时输出完整样式表。
 *
 * @param[in] style  被检查的样式表（QString）。
 * @param[in] needle 期望不出现的 QSS 片段（C 字符串）。
 */
#define EXPECT_QSS_NOT_CONTAINS(style, needle) \
    EXPECT_FALSE(carbonui_test::qssContains((style), (needle)))

#endif // CARBONUI_TESTS_QT_TEST_SUPPORT_H
