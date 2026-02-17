/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "CarbonUIClass.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QDoubleValidator>
#include <QtCore/QLocale>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QToolBar>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>

// helper filter to select all text when the line edit receives focus
class SelectAllOnFocusFilter : public QObject
{
public:
    SelectAllOnFocusFilter(QObject* parent = nullptr) : QObject(parent) {}
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (auto le = qobject_cast<QLineEdit*>(watched))
        {
            if (event->type() == QEvent::FocusIn)
            {
                QTimer::singleShot(0, le, [le]() { le->selectAll(); });
            }
            else if (event->type() == QEvent::MouseButtonPress)
            {
                if (!le->hasFocus())
                {
                    QTimer::singleShot(0, le, [le]() { le->selectAll(); });
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

// internal helper to create common radio visuals
static void setupRadioCommon(QRadioButton* rb, const QString& stylesheet)
{
    rb->setStyleSheet(stylesheet);
    rb->setCursor(Qt::PointingHandCursor);
    rb->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    rb->setContentsMargins(0, 0, 0, 0);

    QString rbText = rb->text();
    rb->setText("");
    auto* centerLabel = new QLabel(rbText, rb);
    centerLabel->setObjectName(rb->objectName() + QStringLiteral("_label"));
    centerLabel->setAlignment(Qt::AlignCenter);
    centerLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    centerLabel->setStyleSheet("color: #cdcdcd;");

    auto* lay = new QHBoxLayout(rb);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(centerLabel, 1, Qt::AlignCenter);

    QObject::connect(rb, &QAbstractButton::toggled, rb, [centerLabel](bool checked)
    {
        centerLabel->setStyleSheet(checked ? "color: #ffffff;" : "color: #cdcdcd;");
    });
}

void CarbonUIClass::applyTooltipStyle(void)
{
    if (qApp)
    {
        QString appStyle = qApp->styleSheet();
        QString ttStyle = "QToolTip { background-color: #0f0f0f; color: #029de8; border: 2px solid #0f0f0f;  }";
        // always append tooltip style so the rounded corners are enforced even if a QToolTip rule exists
        qApp->setStyleSheet(appStyle + "\n" + ttStyle);
    }
}

void CarbonUIClass::applyRadioStyle(QRadioButton* rb, RadioShape shape)
{
    if (!rb) return;

    QString baseStyle =
        "QRadioButton { background-color: #272727; color: #cdcdcd; border-radius: 12px; padding: 2px 5px; "
        "min-width: 28px; min-height: 24px; } "
        "QRadioButton::indicator { width: 0px; height: 0px; margin: 0px; } "
        "QRadioButton:checked { background-color: #009ee8; color: #ffffff; } "
        "QRadioButton:hover { border: 2px solid #cdcdcd; }";

    QString variant;
    switch (shape)
    {
    case Radio_LeftRound:
        variant =
            " border-top-left-radius: 10px; border-bottom-left-radius: 10px;"
            " border-top-right-radius: 0px; border-bottom-right-radius: 0px;";
        break;
    case Radio_RightRound:
        variant =
            " border-top-left-radius: 0px; border-bottom-left-radius: 0px;"
            " border-top-right-radius: 10px; border-bottom-right-radius: 10px;";
        break;
    case Radio_Rectangle:
        variant =
            " border-top-left-radius: 0px; border-bottom-left-radius: 0px;"
            " border-top-right-radius: 0px; border-bottom-right-radius: 0px;";
        break;
    case Radio_FourRound:
    default:
        variant = QString();
        break;
    }

    QString fullStyle = baseStyle;
    if (!variant.isEmpty())
        fullStyle = baseStyle;

    // if rectangle, override the border-radius part in baseStyle by injecting zeros
    if (shape == Radio_Rectangle)
    {
        fullStyle =
            "QRadioButton { background-color: #272727; color: #cdcdcd; padding: 2px 5px; "
            " border-top-left-radius: 0px; border-bottom-left-radius: 0px;"
            " border-top-right-radius: 0px; border-bottom-right-radius: 0px;"
            "min-width: 28px; min-height: 24px; } "
            "QRadioButton::indicator { width: 0px; height: 0px; margin: 0px; } "
            "QRadioButton:checked { background-color: #009ee8; color: #ffffff; } "
            "QRadioButton:hover { border: 2px solid #cdcdcd; }";
    }
    else if (!variant.isEmpty())
    {
        // insert variant immediately after the initial properties
        // baseStyle already contains border-radius: 12px; we want to replace/append variant rules
        // simplest: build style from components
        fullStyle =
            "QRadioButton { background-color: #272727; color: #cdcdcd; border-radius: 12px; padding: 2px 5px; "
            + variant +
            "min-width: 28px; min-height: 24px; } "
            "QRadioButton::indicator { width: 0px; height: 0px; margin: 0px; } "
            "QRadioButton:checked { background-color: #009ee8; color: #ffffff; } "
            "QRadioButton:hover { border: 2px solid #cdcdcd; }";
    }

    setupRadioCommon(rb, fullStyle);
}

void CarbonUIClass::applyLineEditStyle_numberInput(QLineEdit* le, int width)
{
    if (!le) return;

    le->setStyleSheet(
        "QLineEdit { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; border: 2px solid #5f5f5f; border-radius: 12px; padding: 2px 5px; } "
        "QLineEdit:hover { color: #dddddd; border-color: #777777; } ");

    le->setCursor(Qt::IBeamCursor);
    le->setContentsMargins(0, 0, 0, 0);
    le->setAlignment(Qt::AlignCenter);

    if (width > 0)
        le->setFixedWidth(width);

    auto* filter = new SelectAllOnFocusFilter(le);
    le->installEventFilter(filter);

    QObject::connect(le, &QLineEdit::returnPressed, le, [le]() { le->clearFocus(); });

    auto* validator = new QDoubleValidator(-1e12, 1e12, 6, le);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    le->setValidator(validator);
}

void CarbonUIClass::applyCheckStyle(QCheckBox* cb)
{
    if (!cb) return;

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

    class HoverWatcher : public QObject
    {
    public:
        HoverWatcher(QWidget* indicator, QCheckBox* cb) : QObject(cb), m_indicator(indicator), m_cb(cb) {}
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
        QWidget* m_indicator;
        QCheckBox* m_cb;
    };

    auto* watcher = new HoverWatcher(indicator, cb);
    cb->installEventFilter(watcher);
}

void CarbonUIClass::applyScrollAreaStyle(QScrollArea* area)
{
    if (!area) return;

    area->setStyleSheet(
        "QScrollArea { border: none; }"
        "QScrollArea QWidget { background-color: #3b3b3b; }"
        "QScrollArea QScrollBar:vertical { width: 4px; background: transparent; margin: 0px; }"
        "QScrollArea QScrollBar::handle:vertical { background: #cdcdcd; min-height: 20px; border-radius: 4px; }"
        "QScrollArea QScrollBar::add-line:vertical, QScrollArea QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollArea QScrollBar::add-page:vertical, QScrollArea QScrollBar::sub-page:vertical { background: none; }"
    );

    // ensure scrollbars have minimal width
    if (auto* v = area->verticalScrollBar())
    {
        v->setMinimumWidth(4);
    }
}

void CarbonUIClass::applyButtonStyle(QPushButton* btn, ButtonShape shape)
{
    if (!btn) return;

    // determine corner radii depending on shape
    int tl = 0, tr = 0, bl = 0, br = 0;
    switch (shape)
    {
        case Shape_RoundAll: tl = tr = bl = br = 10; break;
        case Shape_LeftRound: tl = bl = 10; tr = br = 0; break;
        case Shape_Rectangle: tl = tr = bl = br = 0; break;
        case Shape_RightRound: tr = br = 10; tl = bl = 0; break;
    }

    QString stylesheet = QString(
        "QPushButton { background-color: #009ee8; color: #ffffff; border: none; padding: 0px 2px; min-height: 28px; "
        "border-top-left-radius: %1px; border-bottom-left-radius: %2px; border-top-right-radius: %3px; border-bottom-right-radius: %4px; }")
        .arg(tl).arg(bl).arg(tr).arg(br);
    stylesheet += "QPushButton:hover { background-color: #00b0ff; }";
    stylesheet += "QPushButton:pressed { background-color: #007bb3; }";

    btn->setStyleSheet(stylesheet);
    btn->setCursor(Qt::PointingHandCursor);
}

void CarbonUIClass::setupMenuBarStyle(QMenuBar* b)
{
	if (!b)
		return;

	// central color for menu background/border so it can be changed easily
	QString menuBg = "#3b3b3b";

	b->setNativeMenuBar(false);
	QFont mbFont = b->font();
	mbFont.setPointSize(10);
	mbFont.setBold(true);
	b->setFont(mbFont);
	b->setFixedHeight(26);

	QString style = QString(
		"QMenuBar { background: %1; color: #cdcdcd; spacing: 0px; padding: 2px 0px; margin: 0px; }"
		"QMenuBar::item { background: transparent; padding: 2px 10px; margin: 0px; border-radius: 0px; }"
		"QMenuBar::item:hover { background: #2e2e2e;  color: #cdcdcd; }"
		"QMenuBar::item:selected { background: #2e2e2e; color: #cdcdcd; }"
		"QMenu { background: #2e2e2e; color: #d0d0d0; border: 1px solid %1; }"
		"QMenu::item:selected { background: %1; color: #cdcdcd; }"
	).arg(menuBg);

	b->setStyleSheet(style);
}

void CarbonUIClass::setupToolBarStyle(QToolBar* tb)
{
    if (!tb) return;

    // set toolbar icon size and add down icon
    tb->setIconSize(QSize(12, 12));
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setFixedHeight(40);

    int iconWidth = tb->iconSize().width();
    int spacing = iconWidth / 2; // horizontal spacing = half icon width
    QString tbStyle = QString(
        "QToolBar { background: #3b3b3b;  border-top: 1px solid #212121; }"
        "QToolBar::handle { width: 0px; }"
        "QToolButton { border: none; border-radius: 10px; padding: 8px; }"
        "QToolButton:hover { border: 2px solid #5d5d5d; }"
    );

    tb->setStyleSheet(tbStyle);

    if (tb->layout())
    {
        tb->layout()->setSpacing(spacing);
    }

    tb->setContentsMargins(32, 0, spacing / 2, 0);

  /*  QAction* downAction = tb->addAction(QIcon(":/images/down.png"), QString());
    downAction->setToolTip("Down");

    QAction* downAction2 = tb->addAction(QIcon(":/images/down.png"), QString());
    downAction2->setToolTip("Down");*/

    // ensure tooltip style for app: dark background and accent text color
    //if (qApp)
    //{
    //    QString appStyle = qApp->styleSheet();
    //    QString ttStyle = "QToolTip { background-color: #0f0f0f; color: #029de8; border: 2px solid #0f0f0f; border-radius: 4px; }";
    //    // always append tooltip style so the rounded corners are enforced even if a QToolTip rule exists
    //    qApp->setStyleSheet(appStyle + "\n" + ttStyle);
    //}
}
