/**
 * @file  screenshot_support.h
 * @brief 截图基线辅助：把控件真实渲染成 PNG，与仓库里的基线图逐像素比对。
 * @author CarbonUI
 * @date  2026-11-06
 * @version 1.0.0
 * @ingroup carbonui_tests
 *
 * @details
 * `tests/visual/carbonui_render_test.cpp` 用"探针像素"验证几个约定好的坐标，
 * 能证明样式生效，但覆盖不到"整幅画面变形"的缺陷（间距漂移、控件重叠、文字被裁、
 * 某个子控件整个没画出来）。本辅助补上这一层：整图与基线比对。
 *
 * 三份图的去向：
 *
 * - 基线：源码树 `tests/visual/baseline/<平台>/<用例>.png`，入库，人工复核后提交；
 * - 实拍：`ui-artifacts/screenshots/<用例>.png`，每次运行覆盖，失败时肉眼对比用；
 * - 差异：`ui-artifacts/screenshots/<用例>-diff.png`，仅失败时生成，红色标出不同像素。
 *
 * 基线必须按平台分目录：字体引擎（DirectWrite / FreeType）与字形提示方式不同，
 * 同一控件在 Windows 与 Linux 上文字的亚像素分布本来就不一样，跨平台共用一份基线
 * 只会得到永久红灯。平台名由编译期宏 `CARBONUI_SCREENSHOT_PLATFORM` 给出。
 *
 * 环境变量（生成/调试基线用，正常跑测试不需要设）：
 *
 * - `CARBONUI_SCREENSHOT_RECORD=1`：记录模式，只补缺失的基线；
 * - `CARBONUI_SCREENSHOT_RECORD=force`：除补缺失外还覆盖已有基线（UI 有意调整后重录用）；
 * - `CARBONUI_SCREENSHOT_BASELINE`：覆盖基线根目录，可用于跨机器排查；
 * - `CARBONUI_SCREENSHOT_CHANNEL_TOLERANCE`：单通道容差，默认 8；
 * - `CARBONUI_SCREENSHOT_MAX_DIFF_RATIO_PERMILLE`：允许的差异像素占比（千分比），默认 5。
 *
 * 容差不是"放水"，而是吸收两件无法避免的事：抗锯齿边缘的舍入差异，以及同平台
 * 不同机器上字体度量的末位差异。真正的 UI 回归（颜色、布局、可见性）会成片命中，
 * 远超千分之五。
 *
 * 线程约定：与所有 Qt Widgets 辅助一样，只在测试主线程调用。
 */

#ifndef CARBONUI_TESTS_SCREENSHOT_SUPPORT_H
#define CARBONUI_TESTS_SCREENSHOT_SUPPORT_H

#include <gtest/gtest.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QLayout>
#include <QtWidgets/QWidget>

namespace carbonui_test
{

/**
 * @brief 一次截图比对的结果，附带可直接打印的说明。
 */
struct ScreenshotCheck
{
    /// 基线是否缺失（缺基线且非记录模式时，用例应跳过而不是失败）。
    bool baselineMissing = false;
    /// 本次是否处于记录模式并已写出基线。
    bool recorded = false;
    /// 与基线是否在容差内一致。
    bool matched = false;
    /// 失败原因或状态说明，直接可读，含文件路径。
    QString message;
    /// 基线图绝对路径。
    QString baselinePath;
    /// 本次实拍图绝对路径。
    QString artifactPath;
    /// 差异图绝对路径；仅在比对失败且已成功写出时非空。
    QString diffPath;
};

/**
 * @brief 取编译期给出的平台名，用于分平台存放基线。
 *
 * @return 形如 windows / linux / macos 的小写平台名；宏缺失时返回 unknown。
 */
inline QString screenshotPlatform()
{
#ifdef CARBONUI_SCREENSHOT_PLATFORM
    return QStringLiteral(CARBONUI_SCREENSHOT_PLATFORM);
#else
    return QStringLiteral("unknown");
#endif
}

/**
 * @brief 基线根目录：默认是源码树里的 `tests/visual/baseline/<平台>`。
 *
 * @return 绝对路径；被环境变量 `CARBONUI_SCREENSHOT_BASELINE` 覆盖时用覆盖值，
 *         此时不再自动追加平台子目录（由调用方自己给出完整语义）。
 */
inline QString screenshotBaselineRoot()
{
    const QString override = qEnvironmentVariable("CARBONUI_SCREENSHOT_BASELINE");
    if (!override.isEmpty())
        return override;

#ifdef CARBONUI_SCREENSHOT_BASELINE_DIR
    return QStringLiteral(CARBONUI_SCREENSHOT_BASELINE_DIR) + QLatin1Char('/') + screenshotPlatform();
#else
    return QString();
#endif
}

/**
 * @brief 实拍图输出目录：测试可执行文件同级的 `ui-artifacts/screenshots`。
 *
 * @return 绝对路径；目录不存在时按需创建，创建失败返回空字符串。
 */
inline QString screenshotArtifactDir()
{
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/ui-artifacts/screenshots");
    if (QDir().mkpath(path))
        return path;
    return QString();
}

/**
 * @brief 是否处于记录基线模式。
 *
 * @return 环境变量 `CARBONUI_SCREENSHOT_RECORD` 非空且不为 0 时返回 true。
 */
inline bool screenshotRecordMode()
{
    // 必须去除空格：在 cmd 里写 "set VAR=1 && next" 会把尾部空格一起设进值。
    const QString value = qEnvironmentVariable("CARBONUI_SCREENSHOT_RECORD").trimmed();
    return !value.isEmpty() && value != QLatin1String("0");
}

/**
 * @brief 是否处于"强制覆盖基线"模式。
 *
 * @details
 * 记录模式默认只补缺失的基线，**绝不覆盖已有基线**——否则一次误设环境变量就会
 * 把"UI 变了"冲掉，回归检测形同虚设。UI 有意调整后需要重录全部基线时，把该变量
 * 设为 force；重录完成后务必复核差异确实只包含本次有意改动。
 *
 * @return 环境变量 `CARBONUI_SCREENSHOT_RECORD` 恰为 force 时返回 true。
 */
inline bool screenshotForceRecord()
{
    return qEnvironmentVariable("CARBONUI_SCREENSHOT_RECORD").trimmed() == QLatin1String("force");
}

/**
 * @brief 读取正整数环境变量，缺失或非法时回落到默认值。
 *
 * @param[in] name         变量名，不可为 NULL。
 * @param[in] defaultValue 未设置或非法时使用的值，必须为正数。
 * @return 解析后的取值。
 */
inline int screenshotEnvInt(const char* name, int defaultValue)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    if (!ok || value <= 0)
        return defaultValue;
    return value;
}

/**
 * @brief 把控件渲染成图片，并把 PNG 落到 `ui-artifacts/screenshots/`。
 *
 * @details
 * 走真实绘制管线（grab 触发 QPainter 与光栅引擎），不调用 show，不会弹窗。
 * 调用方没显式 resize 过时按 sizeHint 收缩，行为与既有渲染用例一致。
 *
 * @param[in,out] widget   待渲染控件（通常是装了被测控件的父容器），不可为 NULL。
 * @param[in]     caseName 用例名，同时作为实拍 PNG 文件名，不可为空字符串。
 * @return 渲染结果，格式固定为 ARGB32；控件为空时返回空图像。
 */
inline QImage captureScreenshot(QWidget* widget, const QString& caseName)
{
    if (!widget)
        return QImage();

    if (!widget->testAttribute(Qt::WA_Resized))
        widget->adjustSize();

    widget->ensurePolished();
    if (QLayout* layout = widget->layout())
        layout->activate();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 30);

    QImage image = widget->grab().toImage().convertToFormat(QImage::Format_ARGB32);

    // 兜底：即使外部设了缩放因子，也把图折回逻辑像素，避免基线与实拍尺寸对不上。
    const qreal dpr = image.devicePixelRatio();
    if (dpr > 1.0)
        image = image.scaled(qRound(image.width() / dpr), qRound(image.height() / dpr),
                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    image.setDevicePixelRatio(1.0);

    const QString dir = screenshotArtifactDir();
    if (!dir.isEmpty())
        image.save(dir + QLatin1Char('/') + caseName + QStringLiteral(".png"));

    return image;
}

/**
 * @brief 比较两张图，统计超容差的像素数与占比。
 *
 * @param[in]  actual    实拍图，不可为空图像。
 * @param[in]  baseline  基线图，不可为空图像。
 * @param[in]  tolerance 单通道允许的最大绝对差，取值 [0, 255]。
 * @param[out] diffImage 差异示意图：相同处为灰、差异处为红；传 NULL 表示不需要。
 * @return 超容差的像素个数；尺寸不一致时返回 -1。
 */
inline qint64 countDifferingPixels(const QImage& actual, const QImage& baseline, int tolerance,
                                   QImage* diffImage)
{
    if (actual.size() != baseline.size())
        return -1;

    QImage diff(actual.size(), QImage::Format_ARGB32);
    qint64 differing = 0;

    for (int y = 0; y < actual.height(); ++y)
    {
        for (int x = 0; x < actual.width(); ++x)
        {
            const QRgb a = actual.pixel(x, y);
            const QRgb b = baseline.pixel(x, y);

            const int delta = qMax(qMax(qAbs(qRed(a) - qRed(b)), qAbs(qGreen(a) - qGreen(b))),
                                   qMax(qAbs(qBlue(a) - qBlue(b)), qAbs(qAlpha(a) - qAlpha(b))));

            const bool different = delta > tolerance;
            if (different)
                ++differing;

            diff.setPixel(x, y, different ? qRgba(255, 0, 0, 255) : qRgba(64, 64, 64, 255));
        }
    }

    if (diffImage)
        *diffImage = diff;

    return differing;
}

/**
 * @brief 抓图并与基线比对，必要时记录基线或写出差异图。
 *
 * @details
 * 判定顺序：缺基线且在记录模式则写基线并算通过；缺基线且非记录模式则标记
 * `baselineMissing`，由用例跳过并提示生成方式；有基线则逐像素比对，
 * 差异占比不超过容差即通过，否则写出差异图并给出可读结论。
 *
 * @param[in,out] widget   待渲染控件，不可为 NULL。
 * @param[in]     caseName 用例名，同时决定基线文件名，不可为空字符串。
 * @return 比对结果；任何一步失败都把原因写进 message。
 */
inline ScreenshotCheck verifyScreenshot(QWidget* widget, const QString& caseName)
{
    ScreenshotCheck check;

    const QString baselineRoot = screenshotBaselineRoot();
    if (baselineRoot.isEmpty())
    {
        check.baselineMissing = true;
        check.message = QStringLiteral("未配置基线目录（编译期缺少 CARBONUI_SCREENSHOT_BASELINE_DIR）");
        return check;
    }

    const QImage actual = captureScreenshot(widget, caseName);
    if (actual.isNull())
    {
        check.message = QStringLiteral("截图失败：控件为空，未得到图像");
        return check;
    }

    const QString artifactDir = screenshotArtifactDir();
    if (!artifactDir.isEmpty())
        check.artifactPath = artifactDir + QLatin1Char('/') + caseName + QStringLiteral(".png");

    check.baselinePath = baselineRoot + QLatin1Char('/') + caseName + QStringLiteral(".png");

    if (!QFileInfo::exists(check.baselinePath) || screenshotForceRecord())
    {
        if (!screenshotRecordMode())
        {
            check.baselineMissing = true;
            check.message = QStringLiteral("缺少基线图 %1；用 CARBONUI_SCREENSHOT_RECORD=1 重跑生成，"
                                           "人工复核后提交")
                                .arg(check.baselinePath);
            return check;
        }

        QDir().mkpath(QFileInfo(check.baselinePath).absolutePath());
        if (!actual.save(check.baselinePath))
        {
            check.message = QStringLiteral("写基线失败：%1").arg(check.baselinePath);
            return check;
        }

        check.recorded = true;
        check.matched = true;
        check.message = QStringLiteral("已记录基线：%1").arg(check.baselinePath);
        return check;
    }

    const QImage baseline = QImage(check.baselinePath).convertToFormat(QImage::Format_ARGB32);
    if (baseline.isNull())
    {
        check.message = QStringLiteral("基线图无法读取：%1").arg(check.baselinePath);
        return check;
    }

    if (baseline.size() != actual.size())
    {
        check.message = QStringLiteral("尺寸不一致：实拍 %1x%2，基线 %3x%4（%5）")
                            .arg(actual.width())
                            .arg(actual.height())
                            .arg(baseline.width())
                            .arg(baseline.height())
                            .arg(check.baselinePath);
        return check;
    }

    const int tolerance = screenshotEnvInt("CARBONUI_SCREENSHOT_CHANNEL_TOLERANCE", 8);
    const int maxRatioPermille = screenshotEnvInt("CARBONUI_SCREENSHOT_MAX_DIFF_RATIO_PERMILLE", 5);

    QImage diffImage;
    const qint64 differing = countDifferingPixels(actual, baseline, tolerance, &diffImage);
    const qint64 total = qint64(actual.width()) * actual.height();
    const double permille = total > 0 ? double(differing) * 1000.0 / double(total) : 0.0;

    if (permille <= double(maxRatioPermille))
    {
        check.matched = true;
        check.message = QStringLiteral("与基线一致：差异 %1/%2 像素（%3‰，容差 %4‰）")
                            .arg(differing)
                            .arg(total)
                            .arg(permille, 0, 'f', 2)
                            .arg(maxRatioPermille);
        return check;
    }

    if (!artifactDir.isEmpty())
    {
        const QString diffPath = artifactDir + QLatin1Char('/') + caseName + QStringLiteral("-diff.png");
        if (diffImage.save(diffPath))
            check.diffPath = diffPath;
    }

    check.message = QStringLiteral("与基线不一致：%1/%2 像素不同（%3‰ > 容差 %4‰）；实拍 %5；基线 %6；差异图 %7")
                        .arg(differing)
                        .arg(total)
                        .arg(permille, 0, 'f', 2)
                        .arg(maxRatioPermille)
                        .arg(check.artifactPath)
                        .arg(check.baselinePath)
                        .arg(check.diffPath.isEmpty() ? QStringLiteral("(写入失败)") : check.diffPath);
    return check;
}

} // namespace carbonui_test

/**
 * @brief 截图并与基线比对：缺基线时跳过用例，不一致时断言失败并打印三份图路径。
 *
 * @details
 * 宏内部负责释放传入的控件，不要在宏之后再 delete。缺基线走 GTEST_SKIP 而不是
 * 失败，是为了让新平台首次接入时不至于一上来红一片——但跳过信息里会明确给出
 * 生成命令，CI 上看到 skip 应当去补基线，而不是当它通过。
 *
 * @param[in] widget   待渲染控件指针（会被 delete），不可为 NULL。
 * @param[in] caseName 用例名（字面量），同时决定基线与实拍文件名。
 */
#define CARBONUI_CHECK_SCREENSHOT(widget, caseName)                                          \
    do                                                                                       \
    {                                                                                        \
        const carbonui_test::ScreenshotCheck carbonuiShotCheck =                             \
            carbonui_test::verifyScreenshot((widget), QStringLiteral(caseName));             \
        delete (widget);                                                                     \
        if (carbonuiShotCheck.baselineMissing)                                               \
            GTEST_SKIP() << carbonuiShotCheck.message.toStdString();                         \
        ASSERT_TRUE(carbonuiShotCheck.matched) << carbonuiShotCheck.message.toStdString();   \
    } while (false)

#endif // CARBONUI_TESTS_SCREENSHOT_SUPPORT_H
