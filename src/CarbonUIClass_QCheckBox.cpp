/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QCheckBox.cpp
 * @brief CarbonUIClass 中复选框样式助手及其内部辅助件的实现。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 复选框的自绘指示灯外观完全靠"压平原生 indicator + 子控件拼圆点"完成，
 * 悬停高亮与左键切换也无法用 QSS 表达，因此本文件维护两个事件过滤器约定：
 * - 左形态（Check_IndicatorLeft）：按钮固定 16x16，外圈描边画在按钮上，
 *   内部 8x8 圆点子控件 `<objectName>_indicator` 由 HoverWatcher 同步换色；
 * - 右形态（Check_IndicatorRight）：文字搬进 `<objectName>_label` 子标签
 *   （由 syncStyledCheckboxText 在重绘/布局时机同步），16x16 圆环子控件
 *   `<objectName>_indicator` 套 8x8 圆点 `<objectName>_dot`，由
 *   RightIndicatorWatcher 接管交互与配色。
 *
 * 公开接口 @ref CarbonUIClass::applyCheckStyle 的完整参数说明写在
 * CarbonUIClass.h；本文件只补充实现细节与内部辅助件的约定。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

namespace
{
/**
 * @brief 复选框的自绘交互过滤器：悬停高亮 + 左键切换 + 指示灯配色。
 *
 * @details
 * QSS 无法表达"悬停时描边变白、勾选时描边变蓝且指示灯同步换色"这套效果，
 * 也无法让 0x0 的原生 indicator 之外再多画一个圆点，因此在复选框上安装
 * 本过滤器接管 Enter / Leave / MouseButtonRelease 三类事件。
 */
class HoverWatcher : public QObject
{
public:
    /**
     * @brief 构造过滤器并挂到复选框上。
     * @param[in] indicator 复选框内部的指示灯子控件，不可为 NULL，不取得所有权。
     * @param[in] cb        被监视的复选框，不可为 NULL，作为本对象的父对象。
     */
    HoverWatcher(QWidget* indicator, QCheckBox* cb) : QObject(cb), m_indicator(indicator), m_cb(cb) {}

    /**
     * @brief 处理悬停与左键释放。
     *
     * @details
     * - Enter：描边改为白色；
     * - Leave：恢复按勾选状态决定的描边色（蓝 / 灰）；
     * - MouseButtonRelease 且为左键：切换勾选状态并**吞掉**事件，避免
     *   QCheckBox 自身的按下/抬起逻辑再切换一次。
     *
     * @param[in] watched 事件接收者，本过滤器只处理复选框自身的事件。
     * @param[in] event   事件对象，不可为 NULL。
     * @return true 表示事件已被处理且不再向下传递（仅左键释放时）；
     *         其余情况返回基类结果 false。
     */
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Enter)
        {
            m_cb->setStyleSheet(
                "QCheckBox { background-color: transparent; border: 2px solid #ffffff; border-radius: 8px;} "
                "QCheckBox::indicator { width: 0px; height: 0px; } "
            );
        }
        else if (event->type() == QEvent::Leave)
        {
            if (m_cb->isChecked())
            {
                m_cb->setStyleSheet(
                    "QCheckBox { background-color: transparent; border: 2px solid #009ee8; border-radius: 8px;} "
                    "QCheckBox::indicator { width: 0px; height: 0px; } "
                );
                m_indicator->setStyleSheet("background-color: transparent; border: 4px solid #009ee8; border-radius: 4px;");
            }
            else
            {
                m_cb->setStyleSheet(
                    "QCheckBox { background-color: transparent; border: 2px solid #636363; border-radius: 8px;} "
                    "QCheckBox::indicator { width: 0px; height: 0px; } "
                );
                m_indicator->setStyleSheet("background-color: transparent; border: 4px solid #636363; border-radius: 4px;");
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton)
            {
                m_cb->setChecked(!m_cb->isChecked());
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }
private:
    QWidget* m_indicator;  ///< 被同步换色的指示灯子控件，不取得所有权。
    QCheckBox* m_cb;       ///< 被监视的复选框，同时是本对象的父对象。
};

/// 指示灯居右形态：圆环描边的三种状态样式（空闲 / 悬停 / 选中）。
constexpr const char* kRightRingIdleStyle =
    "background-color: transparent; border: 2px solid #636363; border-radius: 8px;";
constexpr const char* kRightRingHoverStyle =
    "background-color: transparent; border: 2px solid #ffffff; border-radius: 8px;";
constexpr const char* kRightRingCheckedStyle =
    "background-color: transparent; border: 2px solid #009ee8; border-radius: 8px;";

/**
 * @brief 把复选框文字搬进 `<objectName>_label` 子标签（指示灯居右形态专用）。
 *
 * @details
 * 与单选按钮的 syncStyledRadioText 同一套约定：`text()` 有变化时写入子标签、
 * 清空按钮文字并触发布局失效与重绘；文字为空或标签不存在时什么都不做，
 * 因此可以在 Paint / Show 等高频时机反复调用。
 *
 * @param[in,out] cb 待同步的复选框，**可为 NULL**；为 NULL 或没有文字容器时直接返回。
 */
void syncStyledCheckboxText(QCheckBox* cb)
{
    if (cb == nullptr)
    {
        return;
    }

    QLabel* label = cb->findChild<QLabel*>(
        cb->objectName() + QStringLiteral("_label"), Qt::FindDirectChildrenOnly);
    if (label == nullptr)
    {
        return;
    }

    const QString buttonText = cb->text();
    if (buttonText.isEmpty())
    {
        return;
    }

    label->setText(buttonText);
    cb->setText(QString());
    if (QLayout* layout = cb->layout())
    {
        layout->invalidate();
    }
    if (cb->style() != nullptr)
    {
        cb->style()->unpolish(cb);
        cb->style()->polish(cb);
    }
    cb->updateGeometry();
    cb->update();
}

/**
 * @brief 指示灯居右形态的自绘交互过滤器：悬停高亮 + 左键切换 + 文字同步。
 *
 * @details
 * 与左形态的 HoverWatcher 分工一致，但外观契约不同：外圈描边画在 16x16
 * 圆环子控件上（按钮本身无边框），勾选态只改变内部 8x8 圆点与圆环描边色。
 * 另外承担"文字搬进 `<objectName>_label` 子标签"的同步职责，时机与单选按钮的
 * StyledRadioTextSyncFilter 相同（Paint / Show / Resize / Polish / LayoutRequest /
 * LanguageChange），调用方仍可用 `QCheckBox::setText()` 改文案。
 */
class RightIndicatorWatcher : public QObject
{
public:
    /**
     * @brief 构造过滤器并挂到复选框上。
     * @param[in] ring 圆环子控件，不可为 NULL，不取得所有权。
     * @param[in] dot  圆环内部的圆点子控件，不可为 NULL，不取得所有权。
     * @param[in] cb   被监视的复选框，不可为 NULL，作为本对象的父对象。
     */
    RightIndicatorWatcher(QWidget* ring, QWidget* dot, QCheckBox* cb)
        : QObject(cb), m_ring(ring), m_dot(dot), m_cb(cb)
    {
    }

    /**
     * @brief 处理悬停、左键释放与文字同步。
     *
     * @param[in] watched 事件接收者，本过滤器只处理复选框自身的事件。
     * @param[in] event   事件对象，不可为 NULL。
     * @return true 表示事件已被处理且不再向下传递（仅左键释放时）；
     *         其余情况返回基类结果 false。
     */
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::Enter:
                m_ring->setStyleSheet(QLatin1String(kRightRingHoverStyle));
                break;
            case QEvent::Leave:
                m_ring->setStyleSheet(QLatin1String(
                    m_cb->isChecked() ? kRightRingCheckedStyle : kRightRingIdleStyle));
                break;
            case QEvent::MouseButtonRelease:
            {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton)
                {
                    m_cb->setChecked(!m_cb->isChecked());
                    return true;
                }
                break;
            }
            case QEvent::Paint:
            case QEvent::Show:
            case QEvent::Resize:
            case QEvent::Polish:
            case QEvent::PolishRequest:
            case QEvent::LayoutRequest:
            case QEvent::LanguageChange:
                syncStyledCheckboxText(m_cb);
                break;
            default:
                break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QWidget* m_ring;  ///< 圆环子控件（对应左形态的复选框外圈描边），不取得所有权。
    QWidget* m_dot;   ///< 圆环内部的圆点子控件，不取得所有权。
    QCheckBox* m_cb;  ///< 被监视的复选框，同时是本对象的父对象。
};

/**
 * @brief 指示灯居右形态的装配（形态契约见 CarbonUIClass.h）。
 *
 * @details
 * 圆环 16x16（描边对应左形态的外圈）内部居中放 8x8 圆点；按钮文字搬进
 * `<objectName>_label` 子标签并同步清空 `text()`。按钮内部布局为
 * [标签 | 弹性 | 圆环]：文字左对齐、圆环靠右，多个复选框上下排列时圆环
 * 自然右对齐。按钮宽度不固定，由布局与文字决定。
 *
 * @param[in,out] cb 目标复选框，**不可为 NULL**（调用方已判空）。
 */
void applyCheckStyleIndicatorRight(QCheckBox* cb)
{
    cb->setStyleSheet(
        "QCheckBox { background-color: transparent; border: none; } "
        "QCheckBox::indicator { width: 0px; height: 0px; } "
    );

    cb->setContentsMargins(0, 0, 0, 0);
    cb->setCursor(Qt::PointingHandCursor);
    cb->setAttribute(Qt::WA_Hover, true);
    cb->setMouseTracking(true);

    // 文字搬进 <objectName>_label 子标签（沿用单选按钮的文字容器约定）
    auto* label = new QLabel(cb->text(), cb);
    label->setObjectName(cb->objectName() + QStringLiteral("_label"));
    label->setStyleSheet("color: #cdcdcd; background-color: transparent; ");
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    cb->setText(QString());

    // 16x16 圆环 + 内部居中的 8x8 圆点
    auto* ring = new QWidget(cb);
    ring->setObjectName(cb->objectName() + QStringLiteral("_indicator"));
    ring->setFixedSize(16, 16);
    ring->setAttribute(Qt::WA_TransparentForMouseEvents);
    ring->setStyleSheet(QLatin1String(kRightRingIdleStyle));

    auto* dot = new QWidget(ring);
    dot->setObjectName(cb->objectName() + QStringLiteral("_dot"));
    dot->setFixedSize(8, 8);
    dot->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* ringLayout = new QHBoxLayout(ring);
    ringLayout->setContentsMargins(0, 0, 0, 0);
    ringLayout->addWidget(dot, 0, Qt::AlignCenter);

    auto* lay = new QHBoxLayout(cb);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(label, 1);
    lay->addWidget(ring, 0, Qt::AlignVCenter);

    auto setDotStyle = [dot](bool checked)
    {
        dot->setStyleSheet(
            QString("background-color: transparent; border: 4px solid %1; border-radius: 4px;")
                .arg(checked ? QStringLiteral("#009ee8") : QStringLiteral("#636363")));
    };
    setDotStyle(cb->isChecked());
    QObject::connect(cb, &QAbstractButton::toggled, cb,
                     [setDotStyle](bool checked) { setDotStyle(checked); });

    cb->installEventFilter(new RightIndicatorWatcher(ring, dot, cb));
}
} // namespace


void CarbonUIClass::applyCheckStyle(QCheckBox* cb, CheckIndicatorPosition position)
{
    if (!cb) return;

    if (position == Check_IndicatorRight)
    {
        applyCheckStyleIndicatorRight(cb);
        return;
    }

    cb->setStyleSheet(
        "QCheckBox { background-color: transparent; border: 2px solid #636363; border-radius: 8px;} "
        "QCheckBox::indicator { width: 0px; height: 0px; } "
        "QCheckBox:hover { border: 2px solid #ffffff; } "
        "QCheckBox:checked { border: 2px solid #009ee8; } "
    );

    cb->setContentsMargins(0, 0, 0, 0);
    cb->setFixedSize(16, 16);
    cb->setCursor(Qt::PointingHandCursor);
    cb->setAttribute(Qt::WA_Hover, true);
    cb->setMouseTracking(true);

    QString txt = cb->text();
    Q_UNUSED(txt);
    cb->setText("");
    auto* indicator = new QWidget(cb);
    indicator->setObjectName(cb->objectName() + QStringLiteral("_indicator"));
    int indicatorSize = 8;
    indicator->setFixedSize(indicatorSize, indicatorSize);
    indicator->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* lay = new QHBoxLayout(cb);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(indicator, 0, Qt::AlignCenter);

    auto setIndicatorStyle = [indicator](bool checked, int indicatorSize) {
        indicator->show();
        QString style = QString("background-color: transparent; border: %1px solid %2; border-radius: %3px;")
            .arg(indicatorSize / 2)
            .arg(checked ? QString("#009ee8") : QString("#636363"))
            .arg(indicatorSize / 2);
        indicator->setStyleSheet(style);
    };

    setIndicatorStyle(cb->isChecked(), indicatorSize);
    QObject::connect(cb, &QAbstractButton::toggled, cb, [indicator, indicatorSize](bool checked) {
        QString style = QString("background-color: transparent; border: %1px solid %2; border-radius: %3px;")
            .arg(indicatorSize / 2)
            .arg(checked ? QString("#009ee8") : QString("#636363"))
            .arg(indicatorSize / 2);
        indicator->setStyleSheet(style);
    });


    auto* watcher = new HoverWatcher(indicator, cb);
    cb->installEventFilter(watcher);
}
