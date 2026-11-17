/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQCheckBox.cpp
 * @brief CarbonUIQCheckBox 的外观与内部辅助件实现（契约见 CarbonUIQCheckBox.h）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 复选框的自绘指示灯外观完全靠"压平原生 indicator + 子控件拼圆点"完成，
 * 悬停高亮与左键切换也无法用 QSS 表达，因此本文件维护两个事件过滤器约定：
 * - 左形态（::CarbonUIQCheckBox::Check_IndicatorLeft）：按钮固定 16x16，
 *   外圈描边画在按钮上，内部 8x8 圆点子控件 `<objectName>_indicator` 由
 *   HoverWatcher 同步换色；
 * - 右形态（::CarbonUIQCheckBox::Check_IndicatorRight）：文字搬进
 *   `<objectName>_label` 子标签（由 syncStyledCheckboxText 在重绘/布局时机
 *   同步），16x16 圆环子控件 `<objectName>_indicator` 套 8x8 圆点
 *   `<objectName>_dot`，由 RightIndicatorWatcher 接管交互与配色。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQCheckBox.h"
#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStyle>

namespace {
/**
 * @brief 复选框的自绘交互过滤器：悬停高亮 + 左键切换 + 指示灯配色。
 *
 * @details
 * QSS 无法表达"悬停时描边变白、勾选时描边变蓝且指示灯同步换色"这套效果，
 * 也无法让 0x0 的原生 indicator 之外再多画一个圆点，因此在复选框上安装
 * 本过滤器接管 Enter / Leave / MouseButtonRelease 三类事件。
 */
class HoverWatcher : public QObject {
  public:
	/**
	 * @brief 构造过滤器并挂到复选框上。
	 * @param[in] indicator 复选框内部的指示灯子控件，不可为 NULL，不取得所有权。
	 * @param[in] cb        被监视的复选框，不可为 NULL，作为本对象的父对象。
	 */
	HoverWatcher(QWidget* indicator, QCheckBox* cb)
	    : QObject(cb), m_indicator(indicator), m_cb(cb) {}

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
	bool eventFilter(QObject* watched, QEvent* event) override {
		if (event->type() == QEvent::Enter) {
			m_cb->setStyleSheet("QCheckBox { background-color: transparent; border: 2px solid "
			                    "#ffffff; border-radius: 8px;} "
			                    "QCheckBox::indicator { width: 0px; height: 0px; } ");
		} else if (event->type() == QEvent::Leave) {
			if (m_cb->isChecked()) {
				m_cb->setStyleSheet("QCheckBox { background-color: transparent; border: 2px solid "
				                    "#009ee8; border-radius: 8px;} "
				                    "QCheckBox::indicator { width: 0px; height: 0px; } ");
				m_indicator->setStyleSheet("background-color: transparent; border: 4px solid "
				                           "#009ee8; border-radius: 4px;");
			} else {
				m_cb->setStyleSheet("QCheckBox { background-color: transparent; border: 2px solid "
				                    "#636363; border-radius: 8px;} "
				                    "QCheckBox::indicator { width: 0px; height: 0px; } ");
				m_indicator->setStyleSheet("background-color: transparent; border: 4px solid "
				                           "#636363; border-radius: 4px;");
			}
		} else if (event->type() == QEvent::MouseButtonRelease) {
			auto* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton) {
				m_cb->setChecked(!m_cb->isChecked());
				return true;
			}
		}
		return QObject::eventFilter(watched, event);
	}

  private:
	QWidget* m_indicator; ///< 被同步换色的指示灯子控件，不取得所有权。
	QCheckBox* m_cb;      ///< 被监视的复选框，同时是本对象的父对象。
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
 * 与单选按钮的 syncStyledRadioText 同一套约定：@c text() 有变化时写入子标签、
 * 清空按钮文字并触发布局失效与重绘；文字为空或标签不存在时什么都不做，
 * 因此可以在 Paint / Show 等高频时机反复调用。
 *
 * @param[in,out] cb 待同步的复选框，**可为 NULL**；为 NULL 或没有文字容器时直接返回。
 */
void syncStyledCheckboxText(QCheckBox* cb) {
	if (cb == nullptr) {
		return;
	}

	auto* label = cb->findChild<QLabel*>(cb->objectName() + QStringLiteral("_label"),
	                                     Qt::FindDirectChildrenOnly);
	if (label == nullptr) {
		return;
	}

	const QString buttonText = cb->text();
	if (buttonText.isEmpty()) {
		return;
	}

	label->setText(buttonText);
	cb->setText(QString());
	if (QLayout* layout = cb->layout()) {
		layout->invalidate();
	}
	if (cb->style() != nullptr) {
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
 * LanguageChange），调用方仍可用 @c QCheckBox::setText() 改文案。
 */
class RightIndicatorWatcher : public QObject {
  public:
	/**
	 * @brief 构造过滤器并挂到复选框上。
	 * @param[in] ring 圆环子控件，不可为 NULL，不取得所有权。
	 * @param[in] dot  圆环内部的圆点子控件，不可为 NULL，不取得所有权。
	 * @param[in] cb   被监视的复选框，不可为 NULL，作为本对象的父对象。
	 */
	RightIndicatorWatcher(QWidget* ring, QWidget* dot, QCheckBox* cb)
	    : QObject(cb), m_ring(ring), m_dot(dot), m_cb(cb) {}

	/**
	 * @brief 处理悬停、左键释放与文字同步。
	 *
	 * @param[in] watched 事件接收者，本过滤器只处理复选框自身的事件。
	 * @param[in] event   事件对象，不可为 NULL。
	 * @return true 表示事件已被处理且不再向下传递（仅左键释放时）；
	 *         其余情况返回基类结果 false。
	 */
	bool eventFilter(QObject* watched, QEvent* event) override {
		switch (event->type()) {
		case QEvent::Enter:
			m_ring->setStyleSheet(QLatin1String(kRightRingHoverStyle));
			break;
		case QEvent::Leave:
			m_ring->setStyleSheet(
			    QLatin1String(m_cb->isChecked() ? kRightRingCheckedStyle : kRightRingIdleStyle));
			break;
		case QEvent::MouseButtonRelease: {
			auto* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton) {
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
	QWidget* m_ring; ///< 圆环子控件（对应左形态的复选框外圈描边），不取得所有权。
	QWidget* m_dot;  ///< 圆环内部的圆点子控件，不取得所有权。
	QCheckBox* m_cb; ///< 被监视的复选框，同时是本对象的父对象。
};

/**
 * @brief 指示灯居右形态的装配（形态契约见 CarbonUIQCheckBox.h）。
 *
 * @details
 * 圆环 16x16（描边对应左形态的外圈）内部居中放 8x8 圆点；按钮文字搬进
 * `<objectName>_label` 子标签并同步清空 @c text()。按钮内部布局为
 * [标签 | 弹性 | 圆环]：文字左对齐、圆环靠右，多个复选框上下排列时圆环
 * 自然右对齐。按钮宽度不固定，由布局与文字决定。
 *
 * @param[in,out] cb 目标复选框，**不可为 NULL**（调用方已判空）。
 */
void applyCheckStyleIndicatorRight(QCheckBox* cb) {
	cb->setStyleSheet("QCheckBox { background-color: transparent; border: none; } "
	                  "QCheckBox::indicator { width: 0px; height: 0px; } ");

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

	// QAbstractButton::sizeHint 只按文字/图标算，忽略子布局：文字搬进子标签后
	// sizeHint 退化为 0x0，复选框在布局里会变成看不见的一条（app 截图基线曾
	// 捕获过）。这里显式给最小高度撑开行高，水平方向 Expanding 让圆环贴到
	// 分组右侧，实现"多项上下排列时圆点右对齐"。
	cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	cb->setMinimumHeight(24);

	auto setDotStyle = [dot](bool checked) {
		dot->setStyleSheet(
		    QString("background-color: transparent; border: 4px solid %1; border-radius: 4px;")
		        .arg(checked ? QStringLiteral("#009ee8") : QStringLiteral("#636363")));
	};
	setDotStyle(cb->isChecked());
	QObject::connect(cb, &QAbstractButton::toggled, cb,
	                 [setDotStyle](bool checked) { setDotStyle(checked); });

	cb->installEventFilter(new RightIndicatorWatcher(ring, dot, cb));
}

/**
 * @brief 分段式复选框的自绘交互过滤器：启用态换色 + 左键切换 + 文字同步。
 *
 * @details
 * 外观配色（填充式三态）已全部写进 QSS（:hover / :checked / :disabled 伪态
 * 原生生效），过滤器只承担三件事：原生 indicator 压成 0x0 后点击命中需要
 * 接管（左键释放转给 @c click()，经正常点击管线走 checked 切换，从而尊重
 * 所在 QButtonGroup 的互斥语义）；分段禁用/启用时同步文字子标签颜色
 * （QSS 的 :disabled 伪态管不到子标签）；以及"文字搬进
 * `<objectName>_label` 子标签"的同步，时机与 RightIndicatorWatcher 相同
 * （Paint / Show / Resize / Polish / LayoutRequest / LanguageChange），
 * 调用方仍可用 @c QCheckBox::setText() 改文案。
 */
class SegmentWatcher : public QObject {
  public:
	/**
	 * @brief 构造过滤器并挂到复选框上。
	 * @param[in] label 文字子标签，不可为 NULL，不取得所有权。
	 * @param[in] cb    被监视的复选框，不可为 NULL，作为本对象的父对象。
	 */
	SegmentWatcher(QLabel* label, QCheckBox* cb) : QObject(cb), m_label(label), m_cb(cb) {}

	/**
	 * @brief 处理启用态变化、左键释放与文字同步。
	 *
	 * @param[in] watched 事件接收者，本过滤器只处理复选框自身的事件。
	 * @param[in] event   事件对象，不可为 NULL。
	 * @return true 表示事件已被处理且不再向下传递（仅左键释放时）；
	 *         其余情况返回基类结果 false。
	 */
	bool eventFilter(QObject* watched, QEvent* event) override {
		switch (event->type()) {
		case QEvent::EnabledChange:
			m_label->setStyleSheet(QString("color: %1; background-color: transparent; ")
			                           .arg(m_cb->isEnabled() ? QStringLiteral("#ffffff")
			                                                  : QStringLiteral("#cfcfcf")));
			break;
		case QEvent::MouseButtonRelease: {
			auto* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton) {
				// 走正常点击管线而非直接取反：QButtonGroup exclusive 下点击
				// 已选中段不会取消自己（单选语义），未选中段会同时取消其余段。
				m_cb->click();
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
	QLabel* m_label; ///< 文字子标签，不取得所有权。
	QCheckBox* m_cb; ///< 被监视的复选框，同时是本对象的父对象。
};

/**
 * @brief 把复选框装配成"分段式"外观（供 CarbonUIQCheckButtonGroup 使用）。
 *
 * @details
 * 配色契约以 tools/reference/GroupCheckBox.png 为准：未选中 \#676767 灰底
 * 白字、选中 \#009ee8 蓝底白字，无边框、min-height 28px；悬停在两态上各
 * 加一档提亮（未选中 \#767676 / 选中 \#00b0ff），禁用沿用 \#636363 底
 * \#cfcfcf 字。圆角由四个角参数决定（首段左圆 / 末段右圆 / 中段直角）。
 * 压平原生 indicator，文字搬进居中的 `<objectName>_label` 子标签；左键
 * 切换与启用态换色交给 SegmentWatcher。
 *
 * @param[in,out] cb 目标复选框，**不可为 NULL**（调用方已判空）。
 * @param[in]  tl    左上角圆角，取值 >= 0。
 * @param[in]  tr    右上角圆角，取值 >= 0。
 * @param[in]  br    右下角圆角，取值 >= 0。
 * @param[in]  bl    左下角圆角，取值 >= 0。
 */
void applyCheckStyleSegment(QCheckBox* cb, int tl, int tr, int br, int bl) {
	QString style = QString("QCheckBox { background-color: #676767; color: #ffffff; border: "
	                        "none; padding: 0px 2px; min-height: 28px; "
	                        "border-top-left-radius: %1px; border-bottom-left-radius: %2px; "
	                        "border-top-right-radius: %3px; "
	                        "border-bottom-right-radius: %4px; } "
	                        "QCheckBox::indicator { width: 0px; height: 0px; } ")
	                    .arg(tl)
	                    .arg(bl)
	                    .arg(tr)
	                    .arg(br);
	style += "QCheckBox:hover { background-color: #767676; }";
	style += "QCheckBox:checked { background-color: #009ee8; }";
	style += "QCheckBox:checked:hover { background-color: #00b0ff; }";
	style += "QCheckBox:disabled { background-color: #636363; color: #cfcfcf; }";
	cb->setStyleSheet(style);

	cb->setContentsMargins(0, 0, 0, 0);
	cb->setCursor(Qt::PointingHandCursor);
	cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	cb->setMinimumHeight(28);

	// 文字搬进 <objectName>_label 子标签（沿用复选框的文字容器约定）
	auto* label = new QLabel(cb->text(), cb);
	label->setObjectName(cb->objectName() + QStringLiteral("_label"));
	label->setAlignment(Qt::AlignCenter);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	label->setStyleSheet("color: #ffffff; background-color: transparent; ");
	cb->setText(QString());

	auto* lay = new QHBoxLayout(cb);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->addWidget(label, 0, Qt::AlignCenter);

	cb->installEventFilter(new SegmentWatcher(label, cb));
}
} // namespace

/**
 * @brief 构造复选框并应用形态（详见 CarbonUIQCheckBox.h）。
 */
CarbonUIQCheckBox::CarbonUIQCheckBox(const QString& text, QWidget* parent,
                                     CheckIndicatorPosition position)
    : QCheckBox(text, parent), m_position(position) {
	applyStyle(this, m_position);
}

/**
 * @brief 查询当前指示灯位置形态（详见 CarbonUIQCheckBox.h）。
 */
CarbonUIQCheckBox::CheckIndicatorPosition CarbonUIQCheckBox::indicatorPosition() const {
	return m_position;
}

/**
 * @brief 给已有复选框应用 CarbonUI 外观（详见 CarbonUIQCheckBox.h）。
 */
void CarbonUIQCheckBox::applyStyle(QCheckBox* cb, CheckIndicatorPosition position) {
	if (cb == nullptr) {
		return;
	}

	if (position == Check_IndicatorRight) {
		applyCheckStyleIndicatorRight(cb);
		return;
	}

	cb->setStyleSheet("QCheckBox { background-color: transparent; border: 2px solid #636363; "
	                  "border-radius: 8px;} "
	                  "QCheckBox::indicator { width: 0px; height: 0px; } "
	                  "QCheckBox:hover { border: 2px solid #ffffff; } "
	                  "QCheckBox:checked { border: 2px solid #009ee8; } ");

	cb->setContentsMargins(0, 0, 0, 0);
	cb->setFixedSize(16, 16);
	cb->setCursor(Qt::PointingHandCursor);
	cb->setAttribute(Qt::WA_Hover, true);
	cb->setMouseTracking(true);

	cb->setText("");
	auto* indicator = new QWidget(cb);
	indicator->setObjectName(cb->objectName() + QStringLiteral("_indicator"));
	int indicatorSize = 8;
	indicator->setFixedSize(indicatorSize, indicatorSize);
	indicator->setAttribute(Qt::WA_TransparentForMouseEvents);

	auto* lay = new QHBoxLayout(cb);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->addWidget(indicator, 0, Qt::AlignCenter);

	auto setIndicatorStyle = [indicator, indicatorSize](bool checked) {
		indicator->show();
		QString style =
		    QString("background-color: transparent; border: %1px solid %2; border-radius: %3px;")
		        .arg(indicatorSize / 2)
		        .arg(checked ? QString("#009ee8") : QString("#636363"))
		        .arg(indicatorSize / 2);
		indicator->setStyleSheet(style);
	};

	setIndicatorStyle(cb->isChecked());
	QObject::connect(cb, &QAbstractButton::toggled, cb, setIndicatorStyle);

	cb->installEventFilter(new HoverWatcher(indicator, cb));
}

/**
 * @brief 构造按钮组并装配分段（契约见 CarbonUIQCheckBox.h）。
 *
 * @details
 * 每段一个独立勾选的 QCheckBox，外形由位置决定：首段左两角 10px 圆角、
 * 末段右两角 10px 圆角、中段直角；布局水平间距固定 2px 形成分段缝隙。
 * 全部分段加入一个 exclusive 的 QButtonGroup（以本组为父对象）实现互斥：
 * 点击未选中段选中它并取消其余段，点击已选中段不变；默认选中首段，
 * 构造完成后组内始终有且仅有一个选中项。
 * 组内未设 objectName 时使用默认名，分段与文字标签按组名派生命名。
 *
 * @param[in] segmentCount 分段数量，小于 1 按 1 处理。
 * @param[in] parent       父控件指针，**可为 NULL**。
 */
CarbonUIQCheckButtonGroup::CarbonUIQCheckButtonGroup(int segmentCount, QWidget* parent)
    : QWidget(parent) {
	if (segmentCount < 1) {
		segmentCount = 1;
	}
	if (objectName().isEmpty()) {
		setObjectName(QStringLiteral("carbonUICheckButtonGroup"));
	}

	auto* lay = new QHBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(2);

	auto* exclusiveGroup = new QButtonGroup(this);
	exclusiveGroup->setExclusive(true);

	for (int i = 0; i < segmentCount; ++i) {
		auto* cb = new QCheckBox(this);
		cb->setObjectName(objectName() + QStringLiteral("_segment%1").arg(i));
		// 分段外形与 CarbonUIQPushButton 同约定：首段左圆、末段右圆、中段直角
		int tl = 0;
		int tr = 0;
		int br = 0;
		int bl = 0;
		if (segmentCount == 1) {
			tl = tr = br = bl = 10;
		} else if (i == 0) {
			tl = bl = 10;
		} else if (i == segmentCount - 1) {
			tr = br = 10;
		}
		applyCheckStyleSegment(cb, tl, tr, br, bl);
		exclusiveGroup->addButton(cb);
		lay->addWidget(cb, 1);
		m_segments.append(cb);
	}

	// 默认选中首段：互斥语义下点击已选中段不会取消自己，
	// 因此构造完成后组内始终有且仅有一个选中项。
	m_segments.first()->setChecked(true);
}

/**
 * @brief 统一命名按钮组及其全部分段与文字标签（契约见 CarbonUIQCheckBox.h）。
 *
 * @param[in] name 组 objectName；分段命名为 `<name>_segment<i>`，分段内的
 *                 文字标签随分段改名为 `<分段名>_label`。
 */
void CarbonUIQCheckButtonGroup::setGroupObjectName(const QString& name) {
	setObjectName(name);
	for (int i = 0; i < m_segments.size(); ++i) {
		auto* cb = m_segments.at(i);
		auto* label =
		    cb->findChild<QLabel*>(cb->objectName() + QStringLiteral("_label"),
		                           Qt::FindDirectChildrenOnly);
		cb->setObjectName(name + QStringLiteral("_segment%1").arg(i));
		if (label != nullptr) {
			label->setObjectName(cb->objectName() + QStringLiteral("_label"));
		}
	}
}

/**
 * @brief 查询分段数量（详见 CarbonUIQCheckBox.h）。
 */
int CarbonUIQCheckButtonGroup::segmentCount() const {
	return m_segments.size();
}

/**
 * @brief 按序号取分段复选框（详见 CarbonUIQCheckBox.h）。
 */
QCheckBox* CarbonUIQCheckButtonGroup::segment(int index) const {
	if (index < 0 || index >= m_segments.size()) {
		return nullptr;
	}
	return m_segments.at(index);
}
