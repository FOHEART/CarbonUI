/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  info_cmd.cpp
 * @brief info 命令实现：把版本与渲染环境整理成信封。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#include "info_cmd.h"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>

#include "CarbonUIVersion.h"

namespace carbonui {
namespace cli {

Envelope runInfo(const QStringList& args)
{
    Q_UNUSED(args);

    QJsonObject data;
    data.insert(QStringLiteral("version"), QString::fromLatin1(CARBONUI_VERSION));
    data.insert(QStringLiteral("versionTag"), QString::fromLatin1(CARBONUI_VERSION_TAG));
    data.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
    data.insert(QStringLiteral("platform"),
                QGuiApplication::platformName().isEmpty() ? QLatin1String("unknown")
                                                          : QGuiApplication::platformName());
    data.insert(QStringLiteral("qpa"), QGuiApplication::platformName());
    data.insert(QStringLiteral("defaultFontFamily"), QGuiApplication::font().family());

    // 前 5 个候选字体族：Agent 可比对"字体引擎是否可用"，Linux 上常见
    // 只有 DejaVu 系列，出现空列表说明 fontconfig 没装字体。
    QJsonArray families;
    const QStringList all = QFontDatabase().families();
    for (int i = 0; i < all.size() && i < 5; ++i)
        families.append(all.at(i));
    data.insert(QStringLiteral("fontFamilies"), families);
    data.insert(QStringLiteral("fontFamilyCount"), all.size());

    Envelope env = makeOk(data);
    env.message = QStringLiteral("CarbonUI %1 on Qt %2 (%3)")
                      .arg(QString::fromLatin1(CARBONUI_VERSION),
                           QString::fromLatin1(qVersion()),
                           QGuiApplication::platformName());
    return env;
}

} // namespace cli
} // namespace carbonui
