/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  cli_parser.cpp
 * @brief carbonui_cli 公共工具的实现：信封构造/输出、token 化、枚举映射。
 * @author CarbonUI
 * @date  2026-11-06
 * @ingroup carbonui_cli
 */

#include "CarbonUIQPushButton.h"
#include "CarbonUIQRadioButton.h"

#include "cli_parser.h"

#include <QJsonDocument>
#include <QTextStream>

namespace carbonui {
namespace cli {

Envelope makeOk(const QJsonObject& data)
{
    Envelope env;
    env.ok = true;
    env.code = CodeOk;
    env.message = QStringLiteral("ok");
    env.data = data;
    return env;
}

Envelope makeError(int code, const QString& message)
{
    Envelope env;
    env.ok = false;
    env.code = code;
    env.message = message;
    env.data = QJsonObject();
    return env;
}

QString envelopeJson(const Envelope& env)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("ok"), env.ok);
    obj.insert(QStringLiteral("code"), env.code);
    obj.insert(QStringLiteral("message"), env.message);
    obj.insert(QStringLiteral("data"), env.data);
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void printEnvelope(const Envelope& env, bool json)
{
    QTextStream out(stdout);
    out.setCodec("UTF-8");
    if (json)
    {
        out << envelopeJson(env) << "\n";
        return;
    }

    out << (env.ok ? QStringLiteral("ok") : QStringLiteral("error"))
        << QStringLiteral(" (code=%1): %2").arg(env.code).arg(env.message) << "\n";
    if (env.data.isEmpty())
        return;
    out << QString::fromUtf8(QJsonDocument(env.data).toJson(QJsonDocument::Indented));
}

QStringList tokenizeLine(const QString& line)
{
    // 脚本管道输入常带 UTF-8 BOM，先剥掉，否则首条命令识别失败。
    QString text = line;
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);

    QStringList tokens;
    QString current;
    bool inToken = false;
    QChar quote;
    for (int i = 0; i < text.size(); ++i)
    {
        const QChar ch = text.at(i);
        if (!quote.isNull())
        {
            // 引号内：只有配对的同类引号结束当前 token 内的引用段。
            if (ch == quote)
            {
                quote = QChar();
            }
            else
            {
                current += ch;
                inToken = true;
            }
            continue;
        }
        if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t'))
        {
            if (inToken)
            {
                tokens << current;
                current.clear();
                inToken = false;
            }
            continue;
        }
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\''))
        {
            quote = ch;
            inToken = true;
            continue;
        }
        current += ch;
        inToken = true;
    }
    if (inToken)
        tokens << current;
    return tokens;
}

bool hasFlag(const QStringList& args, const QString& flag)
{
    return args.contains(flag);
}

QString optionValue(const QStringList& args, const QString& name, bool* found)
{
    if (found)
        *found = false;
    const int index = args.indexOf(name);
    if (index < 0)
        return QString();
    if (index + 1 >= args.size())
        return QString(); // 有名无值：调用方用 found == false 报用法错误
    if (found)
        *found = true;
    return args.at(index + 1);
}

QStringList widgetTypes()
{
    // 顺序即 qss --all 的展开顺序，保持稳定以便文档示例可复现。
    return QStringList()
        << QStringLiteral("button")
        << QStringLiteral("radio")
        << QStringLiteral("checkbox")
        << QStringLiteral("lineedit")
        << QStringLiteral("tab")
        << QStringLiteral("combo")
        << QStringLiteral("tree")
        << QStringLiteral("statusbar")
        << QStringLiteral("groupbox");
}

bool parseButtonShape(const QString& text, CarbonUIQPushButton::ButtonShape* shape)
{
    if (!shape)
        return false;
    if (text.isEmpty() || text == QLatin1String("round"))
    {
        *shape = CarbonUIQPushButton::Shape_RoundAll;
        return true;
    }
    if (text == QLatin1String("left"))
    {
        *shape = CarbonUIQPushButton::Shape_LeftRound;
        return true;
    }
    if (text == QLatin1String("right"))
    {
        *shape = CarbonUIQPushButton::Shape_RightRound;
        return true;
    }
    if (text == QLatin1String("rect"))
    {
        *shape = CarbonUIQPushButton::Shape_Rectangle;
        return true;
    }
    return false;
}

bool parseRadioShape(const QString& text, CarbonUIQRadioButton::RadioShape* shape)
{
    if (!shape)
        return false;
    if (text.isEmpty() || text == QLatin1String("round"))
    {
        *shape = CarbonUIQRadioButton::Radio_FourRound;
        return true;
    }
    if (text == QLatin1String("left"))
    {
        *shape = CarbonUIQRadioButton::Radio_LeftRound;
        return true;
    }
    if (text == QLatin1String("right"))
    {
        *shape = CarbonUIQRadioButton::Radio_RightRound;
        return true;
    }
    if (text == QLatin1String("rect"))
    {
        *shape = CarbonUIQRadioButton::Radio_Rectangle;
        return true;
    }
    return false;
}

} // namespace cli
} // namespace carbonui
