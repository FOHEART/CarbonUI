#include "CarbonUIClass.h"
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>


namespace
{
constexpr const char* kCarbonRadioLabelSuffix = "_label";
constexpr const char* kCarbonRadioSyncInstalledProperty = "_carbonui_radio_sync_installed";

QLabel* findCenterLabel(QRadioButton* rb)
{
	if (!rb)
	{
		return nullptr;
	}
	return rb->findChild<QLabel*>(
		rb->objectName() + QLatin1String(kCarbonRadioLabelSuffix), Qt::FindDirectChildrenOnly);
}

void syncStyledRadioText(QRadioButton* rb)
{
	if (!rb)
	{
		return;
	}

	QLabel* centerLabel = findCenterLabel(rb);
	if (!centerLabel)
	{
		return;
	}

	const QString buttonText = rb->text();
	if (buttonText.isEmpty())
	{
		return;
	}

	centerLabel->setText(buttonText);
	rb->setText(QString());
	centerLabel->update();
	centerLabel->adjustSize();
	centerLabel->setGeometry(rb->rect());
	if (QLayout* layout = rb->layout())
	{
		layout->invalidate();
	}
	if (rb->style())
	{
		rb->style()->unpolish(rb);
		rb->style()->polish(rb);
	}
	rb->updateGeometry();
	rb->update();
}

class StyledRadioTextSyncFilter : public QObject
{
public:
	explicit StyledRadioTextSyncFilter(QRadioButton* owner)
		: QObject(owner)
	{
	}

protected:
	bool eventFilter(QObject* watched, QEvent* event) override
	{
		auto* rb = qobject_cast<QRadioButton*>(watched);
		if (!rb)
		{
			return QObject::eventFilter(watched, event);
		}

		switch (event->type())
		{
			case QEvent::Paint:
			case QEvent::Show:
			case QEvent::Resize:
			case QEvent::Polish:
			case QEvent::PolishRequest:
			case QEvent::LayoutRequest:
			case QEvent::LanguageChange:
				syncStyledRadioText(rb);
				break;
			default:
				break;
		}

		return QObject::eventFilter(watched, event);
	}
};
}

// internal helper to create common radio visuals
static void setupRadioCommon(QRadioButton* rb, const QString& stylesheet)
{
	rb->setStyleSheet(stylesheet);
	rb->setCursor(Qt::PointingHandCursor);
	rb->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	rb->setContentsMargins(0, 0, 0, 0);

	QString rbText = rb->text();
	rb->setText("");
	auto* centerLabel = findCenterLabel(rb);
	if (!centerLabel)
	{
		centerLabel = new QLabel(rbText, rb);
		centerLabel->setObjectName(rb->objectName() + QLatin1String(kCarbonRadioLabelSuffix));
		centerLabel->setAlignment(Qt::AlignCenter);
		centerLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
		centerLabel->setStyleSheet("color: #cdcdcd; background-color: transparent; ");

		auto* lay = qobject_cast<QHBoxLayout*>(rb->layout());
		if (!lay)
		{
			lay = new QHBoxLayout(rb);
			lay->setContentsMargins(0, 0, 0, 0);
		}
		lay->addWidget(centerLabel, 1, Qt::AlignCenter);
	}
	else
	{
		centerLabel->setText(rbText.isEmpty() ? centerLabel->text() : rbText);
	}

	QObject::connect(rb, &QAbstractButton::toggled, rb, [centerLabel](bool checked)
	{
		centerLabel->setStyleSheet(checked ? "color: #ffffff; background-color: transparent; " : "color: #cdcdcd; background-color: transparent; ");
	});

	if (!rb->property(kCarbonRadioSyncInstalledProperty).toBool())
	{
		rb->installEventFilter(new StyledRadioTextSyncFilter(rb));
		rb->setProperty(kCarbonRadioSyncInstalledProperty, true);
	}

	syncStyledRadioText(rb);
}

void CarbonUIClass::applyRadioStyle(QRadioButton* rb, RadioShape shape, int minWidth)
{
	if (!rb) return;

	QString minWidthStr = QString("min-width: %1px; min-height: 24px; ").arg(minWidth);

	QString baseStyle =
		"QRadioButton { background-color: #272727; color: #cdcdcd; border-radius: 12px; padding: 2px 5px; "
		+ minWidthStr +
		"} "
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
			+ minWidthStr +
			"} "
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
			minWidthStr +
			"} "
			"QRadioButton::indicator { width: 0px; height: 0px; margin: 0px; } "
			"QRadioButton:checked { background-color: #009ee8; color: #ffffff; } "
			"QRadioButton:hover { border: 2px solid #cdcdcd; }";
	}

	setupRadioCommon(rb, fullStyle);
}