#include "CarbonUIClass.h"
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>


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
	centerLabel->setStyleSheet("color: #cdcdcd; background-color: transparent; ");

	auto* lay = new QHBoxLayout(rb);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->addWidget(centerLabel, 1, Qt::AlignCenter);

	QObject::connect(rb, &QAbstractButton::toggled, rb, [centerLabel](bool checked)
	{
		centerLabel->setStyleSheet(checked ? "color: #ffffff; background-color: transparent; " : "color: #cdcdcd; background-color: transparent; ");
	});
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