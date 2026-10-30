/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  groupboxcollapser.cpp
 * @brief QGroupBoxCollapser 的实现：结构搬迁、标题同步与折叠动画。
 * @author heqingrui
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui
 *
 * @details
 * 本文件实现 groupboxcollapser.h 声明的全部成员。公开接口的参数与返回值语义
 * 写在头文件里，这里只记录实现层面的约定：
 * - 安装流程会就地改写分组框结构，因此**必须在调用方填充完内容后**再调用
 *   installIntoGroupBox()，否则后加入的控件会挂到外层布局上；
 * - 折叠用 `maximumHeight` 属性动画实现：折叠结束时隐藏内容区并把最大高度
 *   重置为 QWIDGETSIZE_MAX，展开结束时同样重置，保证下一次布局能拿到自然高度；
 * - 标题同步走事件过滤器，因此 `groupBox->setTitle()` 之后不需要额外通知。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "groupboxcollapser.h"
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QStyle>
#include <QtCore/QVariant>
#include <QtCore/QEvent>
#include <QtCore/QPropertyAnimation>

// 完整文档在 groupboxcollapser.h，这里不重复 @param/@return，只记录实现顺序。
// 先改样式表，再建控制器（结构搬迁在构造函数里完成），最后按需收起。
QGroupBoxCollapser *QGroupBoxCollapser::installIntoGroupBox(QGroupBox *groupBox, QWidget *cornerWidget, bool expanded)
{
    groupBox->setStyleSheet(
		"QGroupBox { border: none; border-bottom: 1px solid #000000; } "
		"QLabel { color: #ffffff; font-size: 11pt; } "
	);

    auto *p = new QGroupBoxCollapser(groupBox, cornerWidget);

    if (!expanded) {
        QSignalBlocker sb(p->m_toggleButton);
        p->m_toggleButton->setChecked(false);
        p->m_toggleButton->setArrowType(Qt::ArrowType::RightArrow);
        p->m_contentArea->hide();
    }

    return p;
}

void QGroupBoxCollapser::addWidget(QWidget *widget)
{
    if (!widget || !m_contentArea)
    {
        return;
    }

    if (QLayout *layout = m_contentArea->layout())
    {
        layout->addWidget(widget);
        return;
    }

    // 没有布局时退化为"直接挂在内容区上"，由控件自身几何决定位置。
    widget->setParent(m_contentArea);
}

// 内容区在构造函数里接管调用方原有布局；归档文档见 groupboxcollapser.h。
QGroupBoxCollapser::QGroupBoxCollapser(QGroupBox *groupBox, QWidget *cornerWidget)
    : QObject(groupBox), m_groupBox(groupBox), m_titleWidget(nullptr), m_toggleButton(nullptr),
      m_contentArea(nullptr), m_animation(nullptr), m_collapsing(false)
{
    auto *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 内容区接管调用方原有布局，后续所有子控件都挂在这里，折叠时整体隐藏。
    m_contentArea = new QWidget(groupBox);
    m_contentArea->setObjectName("QGroupBoxBody");
    m_contentArea->setLayout(groupBox->layout());

    mainLayout->addWidget(m_contentArea);
    groupBox->setLayout(mainLayout);

    m_toggleButton = new QToolButton(groupBox);
    m_toggleButton->setStyleSheet("QToolButton {font-size: 11pt;  border: none; text-align: left; font-weight: bold; padding: 0; color: #cdcdcd;}");
    m_toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toggleButton->setCheckable(true);
    m_toggleButton->setText(groupBox->title());
    m_toggleButton->setArrowType(Qt::ArrowType::DownArrow);
    m_toggleButton->setChecked(true);
    // make the toggle button expand to fill the title area so clicks anywhere on the title toggle collapse/expand
    m_toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toggleButton->setMinimumWidth(0);

    auto *titleWidget = new QWidget(groupBox);
    titleWidget->setObjectName("QGroupBoxTitle");
    titleWidget->setAttribute(Qt::WA_StyledBackground, true);

    // If the original group box has a stylesheet, copy it to the created widgets so they match
    QString groupStyle = groupBox->styleSheet();
    if (!groupStyle.isEmpty()) {
        // Apply group stylesheet to title and content area
        titleWidget->setStyleSheet(groupStyle);
        m_contentArea->setStyleSheet(groupStyle);
        // Merge group stylesheet with existing toggle button stylesheet to preserve button-specific rules
        QString toggleExisting = m_toggleButton->styleSheet();
        m_toggleButton->setStyleSheet(groupStyle + "\n" + toggleExisting);
    }

    // Setup animation for content area
    m_animation = new QPropertyAnimation(m_contentArea, "maximumHeight", this);
    m_animation->setDuration(180);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);

    // Ensure content area's initial maximumHeight fits its contents
    m_contentArea->setMaximumHeight(m_contentArea->sizeHint().height());

    auto *titleLayout = new QHBoxLayout(titleWidget);
    titleLayout->setContentsMargins(14, 8, 14, 8);
    titleLayout->setSpacing(3);
    titleLayout->setAlignment(Qt::AlignVCenter);
    // make toggle occupy remaining space so its clickable area covers the full title width
    titleLayout->addWidget(m_toggleButton, 1);
    if (cornerWidget)
        titleLayout->addWidget(cornerWidget);

    groupBox->layout()->setMenuBar(titleWidget);
    groupBox->setTitle("");
    groupBox->installEventFilter(this);
   

    connect(m_toggleButton, &QAbstractButton::clicked, this, &QGroupBoxCollapser::toggle);
}

bool QGroupBoxCollapser::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_groupBox)
    {
        switch (event->type())
        {
            case QEvent::LayoutRequest:
            case QEvent::UpdateRequest:
            case QEvent::Paint:
            case QEvent::Polish:
            case QEvent::PolishRequest:
            case QEvent::LanguageChange:
                syncTitleFromGroupBox();
                break;
            default:
                break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void QGroupBoxCollapser::syncTitleFromGroupBox()
{
    if (!m_groupBox || !m_toggleButton)
    {
        return;
    }

    const QString groupTitle = m_groupBox->title();
    if (groupTitle.isEmpty())
    {
        return;
    }

    if (m_toggleButton->text() != groupTitle)
    {
        m_toggleButton->setText(groupTitle);
        m_toggleButton->update();
    }

    m_groupBox->setTitle(QString());
}

void QGroupBoxCollapser::onAnimationFinished()
{
    if (m_collapsing) {
        m_contentArea->hide();
        m_contentArea->setMaximumHeight(QWIDGETSIZE_MAX);
    } else {
        // expanded: reset maximumHeight so layout can manage size naturally
        m_contentArea->setMaximumHeight(QWIDGETSIZE_MAX);
    }
    // disconnect to avoid multiple connections
    disconnect(m_animation, &QPropertyAnimation::finished, this, &QGroupBoxCollapser::onAnimationFinished);
}

void QGroupBoxCollapser::toggle(bool checked)
{
    m_toggleButton->setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);

    if (m_toggleButton->isChecked()) {
        // expand with animation
        m_collapsing = false;
        // ensure the content is visible before measuring
        m_contentArea->show();
        m_animation->stop();
        int startH = 0;
        int endH = m_contentArea->sizeHint().height();
        m_contentArea->setMaximumHeight(startH);
        m_animation->setStartValue(startH);
        m_animation->setEndValue(endH);
        // connect finished to finalize state
        connect(m_animation, &QPropertyAnimation::finished, this, &QGroupBoxCollapser::onAnimationFinished, Qt::UniqueConnection);
        m_animation->start();
    } else {
        // collapse with animation
        m_collapsing = true;
        m_animation->stop();
        int startH = m_contentArea->height();
        if (startH <= 0) startH = m_contentArea->sizeHint().height();
        int endH = 0;
        m_animation->setStartValue(startH);
        m_animation->setEndValue(endH);
        // connect finished to finalize state
        connect(m_animation, &QPropertyAnimation::finished, this, &QGroupBoxCollapser::onAnimationFinished, Qt::UniqueConnection);
        m_animation->start();
    }
}
