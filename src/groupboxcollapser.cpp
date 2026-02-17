/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "groupboxcollapser.h"
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QStyle>
#include <QtCore/QVariant>
#include <QtCore/QPropertyAnimation>

QGroupBoxCollapser *QGroupBoxCollapser::installIntoGroupBox(QGroupBox *groupBox, QWidget *cornerWidget, bool expanded)
{
    auto *p = new QGroupBoxCollapser(groupBox, cornerWidget);

    if (!expanded) {
        QSignalBlocker sb(p->m_toggleButton);
        p->m_toggleButton->setChecked(false);
        p->m_toggleButton->setArrowType(Qt::ArrowType::RightArrow);
        p->m_contentArea->hide();
    }

    return p;
}

QGroupBoxCollapser::QGroupBoxCollapser(QGroupBox *groupBox, QWidget *cornerWidget)
    : QObject(groupBox), m_animation(nullptr), m_collapsing(false)
{
    auto *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

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
   

    connect(m_toggleButton, &QAbstractButton::clicked, this, &QGroupBoxCollapser::toggle);
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
