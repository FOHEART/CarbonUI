/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/*********************************************************************
 * \file   QGroupBoxCollapser.h
 * \brief  
 * 
 * \author heqingrui
 * \date   November 2024
 *********************************************************************/
#ifndef QGROUPBOXCOLLAPSER_H
#define QGROUPBOXCOLLAPSER_H

#include "carbonui_global.h"

#include <QObject>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QWidget>
#include <QtCore/QPropertyAnimation>

class CARBONUI_EXPORT QGroupBoxCollapser : public QObject
{
    Q_OBJECT
public:
    static QGroupBoxCollapser *installIntoGroupBox(QGroupBox *groupBox, QWidget *cornerWidget = nullptr, bool expanded = false);

    void addWidget(QWidget *widget);

private:
    QGroupBoxCollapser(QGroupBox *groupBox, QWidget *cornerWidget = nullptr);
    void toggle(bool checked);

private slots:
    void onAnimationFinished();

private:
    QWidget *m_titleWidget;
    QToolButton *m_toggleButton;
    QWidget *m_contentArea;
    QPropertyAnimation *m_animation;
    bool m_collapsing;
};

#endif // QGROUPBOXCOLLAPSER_H
