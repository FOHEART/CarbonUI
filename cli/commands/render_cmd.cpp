/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  render_cmd.cpp
 * @brief snapshot / gallery 的实现：宿主底板 + 控件摆位 + 抓帧 + 落盘。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#include "render_cmd.h"
#include "widget_factory.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

#include "CarbonUIClass.h"

namespace carbonui {
namespace cli {

namespace {

/// @brief 宿主底板边距（像素），与视觉回归基线的取景习惯一致。
const int kMargin = 8;

/// @brief 从抓到的图像落盘，成功时写 @p data 的公共字段。
Envelope saveShot(const QImage& image, const QString& outPath,
                  const QJsonObject& data, const QString& what)
{
    if (image.isNull())
        return makeError(CodeRender, QStringLiteral("渲染失败：%1 抓到的图像为空").arg(what));
    if (!image.save(outPath, "PNG"))
        return makeError(CodeIo, QStringLiteral("无法写入 %1（目录不存在或无权限）").arg(outPath));

    QJsonObject payload = data;
    payload.insert(QStringLiteral("out"), QDir::toNativeSeparators(outPath));
    payload.insert(QStringLiteral("width"), image.width());
    payload.insert(QStringLiteral("height"), image.height());
    payload.insert(QStringLiteral("bytes"), static_cast<double>(QFileInfo(outPath).size()));
    return makeOk(payload);
}

/// @brief 把控件放到 panel 底板中央，返回可直接 grab() 的宿主。
QWidget* mountOnPanel(QWidget* widget, int extraWidth = 0)
{
    auto* host = new QWidget();
    host->setStyleSheet(QStringLiteral("background-color: #272727;"));
    widget->setParent(host);
    widget->move(kMargin, kMargin);
    host->resize(widget->width() + extraWidth + 2 * kMargin, widget->height() + 2 * kMargin);
    return host;
}

} // namespace

Envelope runSnapshot(const QStringList& args)
{
    WidgetSpec spec;
    bool found = false;

    spec.type = optionValue(args, QStringLiteral("--widget"), &found);
    if (!found)
        spec.type = QStringLiteral("button");
    if (!widgetTypes().contains(spec.type))
        return makeError(CodeUsage, QStringLiteral("未知的 --widget 取值 %1（允许 %2）")
                                       .arg(spec.type, widgetTypes().join(QStringLiteral("|"))));

    spec.shape = optionValue(args, QStringLiteral("--shape"), &found);
    spec.state = optionValue(args, QStringLiteral("--state"), &found);
    if (!found)
        spec.state = QStringLiteral("normal");
    spec.text = optionValue(args, QStringLiteral("--text"), &found);
    // 信封里的 shape 回填默认值，Agent 不必猜"空串是什么外形"。
    if (spec.shape.isEmpty() && (spec.type == QLatin1String("button") || spec.type == QLatin1String("radio")))
        spec.shape = QStringLiteral("round");

    const QString outArg = optionValue(args, QStringLiteral("--out"), &found);

    QString error;
    QWidget* widget = createStyledWidget(spec, &error);
    if (!widget)
        return makeError(CodeUsage, error);

    // 尺寸可被 --width/--height 覆写；groupbox 的高度由折叠控制器决定，忽略覆写。
    int width = 0;
    int height = 0;
    const QString widthArg = optionValue(args, QStringLiteral("--width"), &found);
    if (found)
        width = widthArg.toInt();
    const QString heightArg = optionValue(args, QStringLiteral("--height"), &found);
    if (found)
        height = heightArg.toInt();
    if (width > 0 && height > 0 && spec.type != QLatin1String("groupbox"))
        widget->resize(width, height);

    int extraWidth = 0;
    QLabel* sideLabel = nullptr;
    // applyCheckStyle 把复选框做成 16x16 的纯指示灯（会清空文字），
    // 所以 --text 在场时在右侧补一个次级色标签还原真实用法。
    if (spec.type == QLatin1String("checkbox") && !spec.text.isEmpty())
    {
        sideLabel = new QLabel(spec.text);
        CarbonUIClass::setWidgetLabelsStyleRecursively(sideLabel);
        extraWidth = sideLabel->sizeHint().width() + 8;
    }

    QWidget* host = mountOnPanel(widget, extraWidth);
    if (sideLabel)
    {
        sideLabel->setParent(host);
        sideLabel->move(widget->geometry().right() + 12, kMargin);
        sideLabel->resize(sideLabel->sizeHint());
    }

    const QImage image = host->grab().toImage();
    const QString outPath = outArg.isEmpty()
        ? QStringLiteral("carbonui-%1.png").arg(spec.type)
        : outArg;

    QJsonObject data;
    data.insert(QStringLiteral("widget"), spec.type);
    data.insert(QStringLiteral("shape"), spec.shape);
    data.insert(QStringLiteral("state"), spec.state);

    const Envelope env = saveShot(image, outPath, data, spec.type);
    delete host;
    return env;
}

Envelope runGallery(const QStringList& args)
{
    bool found = false;
    const QString outArg = optionValue(args, QStringLiteral("--out"), &found);

    // 版面与 tests/visual 的 form-overview 基线逐像素同源：同一套控件、
    // 同一套样式函数、同一组坐标（只把品红取景底换成主题面板色）。
    auto* host = new QWidget();
    host->setStyleSheet(QStringLiteral("background-color: #272727;"));
    host->resize(380, 260);

    auto* title = new QLabel(QStringLiteral("CarbonUI controls"), host);
    title->setGeometry(16, 12, 340, 20);

    auto* button = new QPushButton(QStringLiteral("Confirm"), host);
    button->setGeometry(16, 40, 120, 32);
    CarbonUIClass::applyButtonStyle(button);

    auto* checkbox = new QCheckBox(host);
    checkbox->setGeometry(16, 84, 16, 16);
    CarbonUIClass::applyCheckStyle(checkbox);
    checkbox->setChecked(true);

    auto* checkboxLabel = new QLabel(QStringLiteral("Remember me"), host);
    checkboxLabel->setGeometry(40, 84, 160, 16);

    auto* radio = new QRadioButton(QStringLiteral("Mode A"), host);
    radio->setGeometry(16, 116, 120, 28);
    CarbonUIClass::applyRadioStyle(radio, CarbonUIClass::Radio_FourRound, 120);
    radio->setChecked(true);

    auto* lineEdit = new QLineEdit(QStringLiteral("42.5"), host);
    lineEdit->setGeometry(16, 152, 180, 28);
    CarbonUIClass::applyLineEditStyle_numberInput(lineEdit);

    auto* combo = new QComboBox(host);
    combo->setGeometry(16, 188, 180, 28);
    combo->addItem(QStringLiteral("Item one"));
    combo->addItem(QStringLiteral("Item two"));
    CarbonUIClass::setupComboBoxStyle(combo);

    // 标签样式统一最后刷：该函数只处理"当时已存在"的子控件。
    CarbonUIClass::setWidgetLabelsStyleRecursively(host);

    const QImage image = host->grab().toImage();
    const QString outPath = outArg.isEmpty() ? QStringLiteral("carbonui-gallery.png") : outArg;

    const Envelope env = saveShot(image, outPath, QJsonObject(), QStringLiteral("gallery"));
    delete host;
    return env;
}

} // namespace cli
} // namespace carbonui
