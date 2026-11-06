/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  qss_cmd.cpp
 * @brief qss 命令实现：真的建控件、调样式函数、读回 styleSheet()。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#include "qss_cmd.h"
#include "widget_factory.h"

#include <QJsonObject>

namespace carbonui {
namespace cli {

namespace {

/// @brief 建一个控件并取回它被写入的 QSS；失败时 @p error 带原因。
QString styleSheetOf(const WidgetSpec& spec, QString* error)
{
    QWidget* widget = createStyledWidget(spec, error);
    if (!widget)
        return QString();
    const QString qss = widget->styleSheet();
    delete widget;
    return qss;
}

} // namespace

Envelope runQss(const QStringList& args)
{
    bool found = false;
    const bool all = hasFlag(args, QStringLiteral("--all"));

    WidgetSpec spec;
    spec.shape = optionValue(args, QStringLiteral("--shape"), &found);

    if (all)
    {
        QJsonObject data;
        const QStringList types = widgetTypes();
        for (int i = 0; i < types.size(); ++i)
        {
            WidgetSpec one = spec;
            one.type = types.at(i);
            QString error;
            const QString qss = styleSheetOf(one, &error);
            if (!error.isEmpty())
                return makeError(CodeUsage, error);
            data.insert(one.type, qss);
        }
        Envelope env = makeOk(data);
        env.message = QStringLiteral("导出 %1 个控件的 QSS").arg(types.size());
        return env;
    }

    spec.type = optionValue(args, QStringLiteral("--widget"), &found);
    if (!found || spec.type.isEmpty())
        return makeError(CodeUsage, QStringLiteral("缺少 --widget（或用 --all 导出全部）"));
    if (!widgetTypes().contains(spec.type))
        return makeError(CodeUsage, QStringLiteral("未知的 --widget 取值 %1（允许 %2）")
                                       .arg(spec.type, widgetTypes().join(QStringLiteral("|"))));

    QString error;
    const QString qss = styleSheetOf(spec, &error);
    if (!error.isEmpty())
        return makeError(CodeUsage, error);
    if (qss.isEmpty())
        return makeError(CodeUsage, QStringLiteral("控件 %1 没有写入任何 QSS（可能样式由子控件承担）").arg(spec.type));

    QJsonObject data;
    data.insert(QStringLiteral("widget"), spec.type);
    data.insert(QStringLiteral("qss"), qss);
    Envelope env = makeOk(data);
    env.message = QStringLiteral("%1 的 QSS 共 %2 字符").arg(spec.type).arg(qss.size());
    return env;
}

} // namespace cli
} // namespace carbonui
