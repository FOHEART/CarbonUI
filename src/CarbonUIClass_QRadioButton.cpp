/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass_QRadioButton.cpp
 * @brief CarbonUIClass 中单选按钮样式助手及其内部辅助件的实现。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui
 *
 * @details
 * 单选按钮的选中态、分段圆角与文字居中无法只靠 QSS 完成，因此本文件额外维护
 * 一个"文字容器"约定：按钮文字被搬进名为 `<objectName>_label` 的直接子 QLabel，
 * 之后由 StyledRadioTextSyncFilter 在 paint / show / resize / polish 等时机把
 * `QRadioButton::text()` 的变化同步回该标签。
 *
 * 公开接口 @ref CarbonUIClass::applyRadioStyle 的完整参数说明写在
 * CarbonUIClass.h；本文件只补充实现细节与内部辅助件的约定。
 *
 * 线程约定：只允许在 GUI 主线程调用（直接操作 QWidget）。
 */

#include "CarbonUIClass.h"
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>


namespace
{
/// 文字容器标签的名字后缀；完整名字是 `<按钮 objectName> + 该后缀`。
constexpr const char* kCarbonRadioLabelSuffix = "_label";
/// 标记"同步过滤器已安装"的动态属性名，用于避免重复安装。
constexpr const char* kCarbonRadioSyncInstalledProperty = "_carbonui_radio_sync_installed";

/**
 * @brief 查找按钮下的文字容器标签。
 *
 * @details 只在**直接子对象**里查找，避免误取到按钮内部其它层级中同名的标签。
 *
 * @param[in] rb 待查找的单选按钮，**可为 NULL**。
 * @return 找到的标签指针；@p rb 为 NULL 或尚未安装文字容器时返回 NULL。
 */
QLabel* findCenterLabel(QRadioButton* rb)
{
	if (!rb)
	{
		return nullptr;
	}
	return rb->findChild<QLabel*>(
		rb->objectName() + QLatin1String(kCarbonRadioLabelSuffix), Qt::FindDirectChildrenOnly);
}

/**
 * @brief 把按钮文字搬到标签，并就地刷新按钮的布局与样式。
 *
 * @details
 * 流程：(1) 取 `rb->text()`，为空则什么都不做（避免把标签内容擦掉）；
 * (2) 文本写入标签、清空按钮文字；(3) 让标签铺满按钮矩形；
 * (4) 重算布局并 unpolish/polish 一次，使 QSS 中的 `:checked` 等状态重新求值。
 * 之所以要"清空按钮文字"，是因为原生 indicator 已压成 0x0，按钮自绘的文字会与
 * 居中标签重叠。
 *
 * @param[in,out] rb 待同步的单选按钮，**可为 NULL**；为 NULL 或没有文字容器时直接返回。
 */
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

/**
 * @brief 把按钮文字变化同步到文字容器标签的事件过滤器。
 *
 * @details
 * 安装后按钮的 `text()` 有可能被外部改动（调用方改标题、切换语言等）。本过滤器
 * 在 Paint / Show / Resize / Polish / PolishRequest / LayoutRequest /
 * LanguageChange 这些"即将重绘或重新布局"的时机调用 syncStyledRadioText()，
 * 把这些变化搬回居中标签，使调用方无需感知内部结构。
 *
 * 只会安装一次：由 @ref CarbonUIClass::applyRadioStyle 通过动态属性
 * `_carbonui_radio_sync_installed` 去重。
 */
class StyledRadioTextSyncFilter : public QObject
{
public:
	/**
	 * @brief 构造过滤器。
	 * @param[in] owner 被监视的单选按钮，不可为 NULL，同时作为本对象的父对象。
	 */
	explicit StyledRadioTextSyncFilter(QRadioButton* owner)
		: QObject(owner)
	{
	}

protected:
	/**
	 * @brief 在重绘/布局相关事件上触发文字同步。
	 *
	 * @param[in] watched 事件接收者，期望是 QRadioButton；其它类型原样放行。
	 * @param[in] event   事件对象，不可为 NULL。
	 * @return 恒定返回 false，事件继续交给按钮自身处理。
	 */
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

/**
 * @brief 应用所有单选按钮外形共用的部分：样式表、指针光标、文字搬迁与过滤器。
 *
 * @details
 * 先写样式表与几何策略，再把 `text()` 搬进 `<objectName>_label` 子标签
 * （没有就新建，有就复用），最后接上"勾选变色"槽并保证同步过滤器只装一次。
 * 外形差异只体现在 @p stylesheet 里，因此本函数被 applyRadioStyle() 的四种
 * 形状共用。
 *
 * @param[in,out] rb         目标单选按钮，**不可为 NULL**（调用方已判空）。
 * @param[in]     stylesheet 完整的 QSS 文本，不可为空字符串；由调用方按形状拼好。
 */
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
