/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQRadioButton.cpp
 * @brief CarbonUIQRadioButton 的外观与内部辅助件实现（契约见
 *        CarbonUIQRadioButton.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 单选按钮的选中态、分段圆角与文字居中无法只靠 QSS 完成，因此本文件额外维护
 * 一个"文字容器"约定：按钮文字被搬进名为 `<objectName>_label` 的直接子 QLabel，
 * 之后由 StyledRadioTextSyncFilter 在 paint / show / resize / polish 等时机把
 * @c QRadioButton::text() 的变化同步回该标签。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQRadioButton.h"
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtGui/QPainter>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QStyle>

namespace {
/// 文字容器标签的名字后缀；完整名字是 `<按钮 objectName> + 该后缀`。
constexpr const char* kCarbonRadioLabelSuffix = "_label";
/// 标记"同步过滤器已安装"的动态属性名，用于避免重复安装。
constexpr const char* kCarbonRadioSyncInstalledProperty = "_carbonui_radio_sync_installed";

/**
 * @brief 单选按钮的文字容器标签：铺满按钮内容区并对超宽文本做省略绘制。
 *
 * @details
 * 旧实现把标签按文本宽度 adjustSize 后再 setGeometry 到按钮矩形，两股力量
 * 在布局失效时互相打架：布局以"带对齐的项取 sizeHint"重排，长文本标签被
 * 撑到比按钮更宽并居中，文字就越出了按钮边界。
 * 本类改为纯布局驱动：
 *  - 水平/垂直尺寸策略 Ignored，布局把它铺满整个格子（即按钮内容区）；
 *  - 超出宽度的文本在绘制阶段按字体度量省略（ElideRight），从机制上保证
 *    文字永远落在按钮矩形之内。
 * 颜色沿用样式表的 color 规则（polish 阶段写入调色板 WindowText），
 * 选中/未选中的变色机制不受影响。
 */
class CarbonRadioTextLabel : public QLabel {
  public:
	/**
	 * @brief 构造文字容器标签。
	 * @param[in] text 初始文案，可为空字符串。
	 * @param[in] parent 父控件（宿主单选按钮），不可为 NULL。
	 */
	explicit CarbonRadioTextLabel(const QString& text, QWidget* parent) : QLabel(text, parent) {
		setAlignment(Qt::AlignCenter);
		setAttribute(Qt::WA_TransparentForMouseEvents);
		// 铺满按钮内容区：布局给多少就用多少，文本在绘制时自行省略
		setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	}

  protected:
	/**
	 * @brief 省略绘制：超宽文本以省略号截断并居中画出。
	 * @param[in] event 绘制事件对象，不可为 NULL。
	 */
	void paintEvent(QPaintEvent* event) override {
		Q_UNUSED(event);
		QPainter painter(this);
		const QString elided = fontMetrics().elidedText(text(), Qt::ElideRight, width());
		painter.setPen(palette().color(QPalette::WindowText));
		painter.drawText(rect(), alignment(), elided);
	}
};

/**
 * @brief 查找按钮下的文字容器标签。
 *
 * @details 只在**直接子对象**里查找，避免误取到按钮内部其它层级中同名的标签。
 *
 * @param[in] rb 待查找的单选按钮，**可为 NULL**。
 * @return 找到的标签指针；@p rb 为 NULL 或尚未安装文字容器时返回 NULL。
 */
QLabel* findCenterLabel(QRadioButton* rb) {
	if (rb == nullptr) {
		return nullptr;
	}
	return rb->findChild<QLabel*>(rb->objectName() + QLatin1String(kCarbonRadioLabelSuffix),
	                              Qt::FindDirectChildrenOnly);
}

/**
 * @brief 把按钮文字搬到标签，并就地刷新按钮的布局与样式。
 *
 * @details
 * 流程：(1) 取 @c rb->text()，为空则什么都不做（避免把标签内容擦掉）；
 * (2) 文本写入标签、清空按钮文字；(3) 触发布局失效与重绘。
 * 标签的几何完全交由布局管理（Ignored 尺寸策略铺满按钮内容区，见
 * CarbonRadioTextLabel），这里不再手工 adjustSize/setGeometry——旧实现的
 * 这两步会与布局重新求值打架，长文本标签被撑出按钮边界导致文字越界。
 * 之所以要"清空按钮文字"，是因为原生 indicator 已压成 0x0，按钮自绘的文字会与
 * 居中标签重叠。
 *
 * @param[in,out] rb 待同步的单选按钮，**可为 NULL**；为 NULL 或没有文字容器时直接返回。
 */
void syncStyledRadioText(QRadioButton* rb) {
	if (rb == nullptr) {
		return;
	}

	QLabel* centerLabel = findCenterLabel(rb);
	if (centerLabel == nullptr) {
		return;
	}

	const QString buttonText = rb->text();
	if (buttonText.isEmpty()) {
		return;
	}

	centerLabel->setText(buttonText);
	rb->setText(QString());
	centerLabel->update();
	if (QLayout* layout = rb->layout()) {
		layout->invalidate();
	}
	if (rb->style() != nullptr) {
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
 * 安装后按钮的 @c text() 有可能被外部改动（调用方改标题、切换语言等）。本过滤器
 * 在 Paint / Show / Resize / Polish / PolishRequest / LayoutRequest /
 * LanguageChange 这些"即将重绘或重新布局"的时机调用 syncStyledRadioText()，
 * 把这些变化搬回居中标签，使调用方无需感知内部结构。
 *
 * 只会安装一次：由 CarbonUIQRadioButton::applyStyle 通过动态属性
 * `_carbonui_radio_sync_installed` 去重。
 */
class StyledRadioTextSyncFilter : public QObject {
  public:
	/**
	 * @brief 构造过滤器。
	 * @param[in] owner 被监视的单选按钮，不可为 NULL，同时作为本对象的父对象。
	 */
	explicit StyledRadioTextSyncFilter(QRadioButton* owner) : QObject(owner) {}

  protected:
	/**
	 * @brief 在重绘/布局相关事件上触发文字同步。
	 *
	 * @param[in] watched 事件接收者，期望是 QRadioButton；其它类型原样放行。
	 * @param[in] event   事件对象，不可为 NULL。
	 * @return 恒定返回 false，事件继续交给按钮自身处理。
	 */
	bool eventFilter(QObject* watched, QEvent* event) override {
		auto* rb = qobject_cast<QRadioButton*>(watched);
		if (rb == nullptr) {
			return QObject::eventFilter(watched, event);
		}

		switch (event->type()) {
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

/**
 * @brief 应用所有单选按钮外形共用的部分：样式表、指针光标、文字搬迁与过滤器。
 *
 * @details
 * 先写样式表与几何策略，再把 @c text() 搬进 `<objectName>_label` 子标签
 * （没有就新建，有就复用），最后接上"勾选变色"槽并保证同步过滤器只装一次。
 * 外形差异只体现在 @p stylesheet 里，因此本函数被四种形状共用。
 *
 * @param[in,out] rb         目标单选按钮，**不可为 NULL**（调用方已判空）。
 * @param[in]     stylesheet 完整的 QSS 文本，不可为空字符串；由调用方按形状拼好。
 */
void setupRadioCommon(QRadioButton* rb, const QString& stylesheet) {
	rb->setStyleSheet(stylesheet);
	rb->setCursor(Qt::PointingHandCursor);
	rb->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	rb->setContentsMargins(0, 0, 0, 0);

	QString rbText = rb->text();
	rb->setText("");
	auto* centerLabel = findCenterLabel(rb);
	if (centerLabel == nullptr) {
		// 省略绘制 + 铺满按钮内容区的文字容器（见 CarbonRadioTextLabel）
		centerLabel = new CarbonRadioTextLabel(rbText, rb);
		centerLabel->setObjectName(rb->objectName() + QLatin1String(kCarbonRadioLabelSuffix));
		centerLabel->setStyleSheet("color: #cdcdcd; background-color: transparent; ");

		auto* lay = qobject_cast<QHBoxLayout*>(rb->layout());
		if (lay == nullptr) {
			lay = new QHBoxLayout(rb);
			lay->setContentsMargins(0, 0, 0, 0);
		}
		// 不带对齐地加入：标签铺满整个格子；带对齐的项会被布局按
		// sizeHint 缩放居中，正是长文本越界的来源
		lay->addWidget(centerLabel, 1);
	} else {
		centerLabel->setText(rbText.isEmpty() ? centerLabel->text() : rbText);
	}

	QObject::connect(rb, &QAbstractButton::toggled, rb, [centerLabel](bool checked) {
		centerLabel->setStyleSheet(checked ? "color: #ffffff; background-color: transparent; "
		                                   : "color: #cdcdcd; background-color: transparent; ");
	});

	if (!rb->property(kCarbonRadioSyncInstalledProperty).toBool()) {
		rb->installEventFilter(new StyledRadioTextSyncFilter(rb));
		rb->setProperty(kCarbonRadioSyncInstalledProperty, true);
	}

	syncStyledRadioText(rb);
}
} // namespace

/**
 * @brief 构造单选按钮并应用外形（详见 CarbonUIQRadioButton.h）。
 */
CarbonUIQRadioButton::CarbonUIQRadioButton(const QString& text, QWidget* parent, RadioShape shape,
                                           int minWidth)
    : QRadioButton(text, parent), m_shape(shape) {
	applyStyle(this, m_shape, minWidth);
}

/**
 * @brief 切换外形与最小宽度并重刷样式表（详见 CarbonUIQRadioButton.h）。
 */
void CarbonUIQRadioButton::setRadioShape(RadioShape shape, int minWidth) {
	m_shape = shape;
	applyStyle(this, m_shape, minWidth);
}

/**
 * @brief 查询当前外形（详见 CarbonUIQRadioButton.h）。
 */
CarbonUIQRadioButton::RadioShape CarbonUIQRadioButton::radioShape() const {
	return m_shape;
}

/**
 * @brief 给已有单选按钮应用 CarbonUI 外观（详见 CarbonUIQRadioButton.h）。
 */
void CarbonUIQRadioButton::applyStyle(QRadioButton* rb, RadioShape shape, int minWidth) {
	if (rb == nullptr) {
		return;
	}

	QString minWidthStr = QString("min-width: %1px; min-height: 24px; ").arg(minWidth);

	QString variant;
	switch (shape) {
	case Radio_LeftRound:
		variant = " border-top-left-radius: 10px; border-bottom-left-radius: 10px;"
		          " border-top-right-radius: 0px; border-bottom-right-radius: 0px;";
		break;
	case Radio_RightRound:
		variant = " border-top-left-radius: 0px; border-bottom-left-radius: 0px;"
		          " border-top-right-radius: 10px; border-bottom-right-radius: 10px;";
		break;
	case Radio_Rectangle:
		variant = " border-top-left-radius: 0px; border-bottom-left-radius: 0px;"
		          " border-top-right-radius: 0px; border-bottom-right-radius: 0px;";
		break;
	case Radio_FourRound:
	default:
		variant = QString();
		break;
	}

	QString fullStyle =
	    "QRadioButton { background-color: #272727; color: #cdcdcd; padding: 2px 5px; " + variant +
	    minWidthStr +
	    "} "
	    "QRadioButton::indicator { width: 0px; height: 0px; margin: 0px; } "
	    "QRadioButton:checked { background-color: #009ee8; color: #ffffff; } "
	    "QRadioButton:hover { border: 2px solid #cdcdcd; }";
	if (variant.isEmpty()) {
		// 四角圆角形态：variant 为空串时补回默认 12px 圆角
		fullStyle = "QRadioButton { background-color: #272727; color: #cdcdcd; border-radius: "
		            "12px; padding: 2px 5px; " +
		            minWidthStr +
		            "} "
		            "QRadioButton::indicator { width: 0px; height: 0px; margin: 0px; } "
		            "QRadioButton:checked { background-color: #009ee8; color: #ffffff; } "
		            "QRadioButton:hover { border: 2px solid #cdcdcd; }";
	}

	setupRadioCommon(rb, fullStyle);
}
