/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIColorPickWidget.cpp
 * @brief CarbonUIColorPickWidget 的外观与交互实现（外观契约见 CarbonUIColorPickWidget.h）。
 * @author CarbonUI
 * @date  2026-11-17
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 选色区（ColorAreaWidget）与明度滑条（ValueSliderWidget）是本文件内的私有
 * QWidget 子类：不做信号声明，因此不需要 Q_OBJECT / moc，取色结果经
 * std::function 回调交还 CarbonUIColorPickWidget 统一处理。配色与输入框
 * QSS 与其它 CarbonUIQ* 控件同一套面板色（\#2f2f2f / \#cdcdcd / \#5f5f5f），
 * 强调色为库主色 \#009ee8。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIColorPickWidget.h"

#include <functional>

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

namespace {

/// 选色区固定尺寸（像素）：横向色相 × 纵向饱和度，底部为白色。
const int kAreaWidth = 258;
const int kAreaHeight = 230;
/// 明度滑条固定尺寸（像素）。
const int kSliderWidth = 26;
/// 右列（按钮 + 输入框）统一宽度（像素）。
const int kColumnWidth = 104;

/**
 * @brief 把透明度百分比（0..100）换算成 0..255 的 alpha 字节。
 */
inline int alphaFromPercent(int percent) {
	return qRound(percent * 255.0 / 100.0);
}

/**
 * @brief 把 0..255 的 alpha 字节换算成透明度百分比（0..100）。
 */
inline int percentFromAlpha(int alpha) {
	return qRound(alpha * 100.0 / 255.0);
}

/**
 * @brief 按参考界面把数值裁剪回 [0, 1]（鼠标取值可能越界）。
 */
inline double clamp01(double v) {
	if (v < 0.0) {
		return 0.0;
	}
	if (v > 1.0) {
		return 1.0;
	}
	return v;
}

/**
 * @brief 组装一个通道输入胶囊（布局与参考图 ColorPickWidget.png 一致）：
 *        字母左端对齐 + 垂直分割线 + 分割线右侧数字居中对齐。
 * @param[in] letter     通道字母（R/G/B/A）。
 * @param[in] objectName 内部 QSpinBox 的 objectName。
 * @param[in] maxValue   通道取值上限（RGB 为 255、透明度为 100）。
 * @param[in,out] parent 胶囊的父控件。
 * @param[out] outBox    返回内部 QSpinBox 指针，不可为 NULL。
 * @return 胶囊容器（父对象为 @p parent，交由调用方加入布局）。
 */
QWidget* makeChannelPill(const char* letter, const char* objectName, int maxValue, QWidget* parent,
                         QSpinBox** outBox) {
	QSpinBox* box = new QSpinBox(parent);
	box->setObjectName(QLatin1String(objectName));
	box->setRange(0, maxValue);
	box->setButtonSymbols(QAbstractSpinBox::NoButtons);
	box->setFrame(false);
	box->setAlignment(Qt::AlignCenter);

	auto* pill = new QWidget(parent);
	pill->setObjectName(QLatin1String("carbonUIColorPickChannel"));
	pill->setAttribute(Qt::WA_StyledBackground, true); // 纯 QWidget 须显式开启才绘制 QSS 背景
	pill->setFixedHeight(24);
	auto* pillLayout = new QHBoxLayout(pill);
	pillLayout->setContentsMargins(10, 0, 6, 0);
	pillLayout->setSpacing(5);
	auto* letterLabel = new QLabel(QLatin1String(letter), pill);
	letterLabel->setFixedWidth(10);
	pillLayout->addWidget(letterLabel);
	auto* divider = new QWidget(pill);
	divider->setFixedSize(1, 12);
	divider->setAttribute(Qt::WA_StyledBackground, true);
	divider->setStyleSheet(QStringLiteral("background-color: #5f5f5f;"));
	pillLayout->addWidget(divider);
	pillLayout->addWidget(box, 1);
	*outBox = box;
	return pill;
}
} // namespace

/**
 * @brief 色相 × 饱和度选色区：横向色相彩虹，纵向由上（满饱和）到下（白色）渐变，
 *        始终以满明度绘制（明度由右侧滑条单独承担，参考图 RGB=0 时选色区仍是
 *        全彩彩虹）；圆环光标指示当前取色点。
 *
 * @details 不声明信号（无需 moc），取色结果经 @ref onPick 回调上交。
 */
class ColorAreaWidget : public QWidget {
  public:
	/// @brief 构造选色区：固定尺寸并开启鼠标跟踪。
	/// @param[in] parent 父控件指针，可为 NULL。
	explicit ColorAreaWidget(QWidget* parent) : QWidget(parent), hue_(0.0), sat_(0.0) {
		setFixedSize(kAreaWidth, kAreaHeight);
		setMouseTracking(true);
	}

	std::function<void(double hue, double sat)> onPick; ///< 取色回调（色相/饱和度均已归一化）。

	/**
	 * @brief 同步光标的色相/饱和度并重绘。
	 * @param[in] hue 色相 [0, 1]。
	 * @param[in] sat 饱和度 [0, 1]。
	 */
	void setHsv(double hue, double sat) {
		hue_ = clamp01(hue);
		sat_ = clamp01(sat);
		update();
	}

	/// @brief 当前色相 [0, 1]。
	double hue() const { return hue_; }

	/// @brief 当前饱和度 [0, 1]。
	double sat() const { return sat_; }

  protected:
	/// @brief 绘制色相/饱和度渐变与圆环光标。
	/// @param[in] event 事件参数（未使用）。
	void paintEvent(QPaintEvent* event) override {
		Q_UNUSED(event);
		QPainter painter(this);
		// 横向色相彩虹（红->黄->绿->青->蓝->品红->红）
		QLinearGradient hueGradient(0.0, 0.0, (double)width(), 0.0);
		for (int i = 0; i <= 6; ++i) {
			hueGradient.setColorAt(i / 6.0, QColor::fromHsvF(i / 6.0, 1.0, 1.0));
		}
		painter.fillRect(rect(), hueGradient);
		// 纵向饱和度：顶部全彩、底部叠白
		QLinearGradient satGradient(0.0, 0.0, 0.0, (double)height());
		satGradient.setColorAt(0.0, QColor(255, 255, 255, 0));
		satGradient.setColorAt(1.0, QColor(255, 255, 255, 255));
		painter.fillRect(rect(), satGradient);
		// 光标：白圈 + 黑描边，保证在亮暗底色上都可见
		const QPointF cursor(hue_ * (width() - 1), (1.0 - sat_) * (height() - 1));
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(QPen(Qt::black, 2.0));
		painter.setBrush(Qt::NoBrush);
		painter.drawEllipse(cursor, 6.0, 6.0);
		painter.setPen(QPen(Qt::white, 1.0));
		painter.drawEllipse(cursor, 6.0, 6.0);
	}

	/// @brief 按下即取色。
	/// @param[in] event 鼠标事件。
	void mousePressEvent(QMouseEvent* event) override { pick(event->pos()); }

	/// @brief 左键拖动持续取色。
	/// @param[in] event 鼠标事件。
	void mouseMoveEvent(QMouseEvent* event) override {
		if (event->buttons() & Qt::LeftButton) {
			pick(event->pos());
		}
	}

  private:
	/**
	 * @brief 鼠标取色：坐标换算成色相/饱和度后回调上交。
	 * @param[in] pos 控件坐标，越界值按边界裁剪。
	 */
	void pick(const QPoint& pos) {
		const double hue = clamp01((double)pos.x() / (double)(width() - 1));
		const double sat = 1.0 - clamp01((double)pos.y() / (double)(height() - 1));
		if (onPick) {
			onPick(hue, sat);
		}
	}

	double hue_; ///< 当前色相 [0, 1]。
	double sat_; ///< 当前饱和度 [0, 1]。
};

/**
 * @brief 纵向明度滑条：上白下黑渐变，横线光标指示当前明度。
 *
 * @details 不声明信号（无需 moc），取值结果经 @ref onPick 回调上交。
 */
class ValueSliderWidget : public QWidget {
  public:
	/// @brief 构造明度滑条：固定尺寸并开启鼠标跟踪。
	/// @param[in] parent 父控件指针，可为 NULL。
	explicit ValueSliderWidget(QWidget* parent) : QWidget(parent), val_(1.0) {
		setFixedSize(kSliderWidth, kAreaHeight);
		setMouseTracking(true);
	}

	std::function<void(double val)> onPick; ///< 取值回调（明度已归一化）。

	/**
	 * @brief 同步明度并重绘。
	 * @param[in] val 明度 [0, 1]，越界值按边界裁剪。
	 */
	void setValue(double val) {
		val_ = clamp01(val);
		update();
	}

	/// @brief 当前明度 [0, 1]。
	double value() const { return val_; }

  protected:
	/// @brief 绘制上白下黑渐变与明度横线光标。
	/// @param[in] event 事件参数（未使用）。
	void paintEvent(QPaintEvent* event) override {
		Q_UNUSED(event);
		QPainter painter(this);
		QLinearGradient gradient(0.0, 0.0, 0.0, (double)height());
		gradient.setColorAt(0.0, QColor(255, 255, 255));
		gradient.setColorAt(1.0, QColor(0, 0, 0));
		painter.fillRect(rect(), gradient);
		// 光标：当前明度处的横线（白线 + 黑描边）。渐变上白（明度 1）下黑（明度 0），
		// 光标必须同向映射：明度 0 画在最下端。
		const double y = (1.0 - val_) * (height() - 1);
		painter.setPen(QPen(Qt::black, 3.0));
		painter.drawLine(0, qRound(y), width() - 1, qRound(y));
		painter.setPen(QPen(Qt::white, 1.0));
		painter.drawLine(0, qRound(y), width() - 1, qRound(y));
	}

	/// @brief 按下即取值。
	/// @param[in] event 鼠标事件。
	void mousePressEvent(QMouseEvent* event) override { pick(event->pos()); }

	/// @brief 左键拖动持续取值。
	/// @param[in] event 鼠标事件。
	void mouseMoveEvent(QMouseEvent* event) override {
		if (event->buttons() & Qt::LeftButton) {
			pick(event->pos());
		}
	}

  private:
	/**
	 * @brief 鼠标取值：纵坐标换算成明度后回调上交。
	 * @param[in] pos 控件坐标，越界值按边界裁剪。
	 */
	void pick(const QPoint& pos) {
		const double val = 1.0 - clamp01((double)pos.y() / (double)(height() - 1));
		if (onPick) {
			onPick(val);
		}
	}

	double val_; ///< 当前明度 [0, 1]。
};

/**
 * @brief 构造取色面板并完成外观装配（详见 CarbonUIColorPickWidget.h）。
 */
CarbonUIColorPickWidget::CarbonUIColorPickWidget(QWidget* parent) : QWidget(parent) {
	// 面板底色/边框/圆角全部走 QSS：纯 QWidget 子类须显式开启才绘制样式表背景
	setAttribute(Qt::WA_StyledBackground, true);
	buildUi();
	setColor(Qt::black);
}

/**
 * @brief 查询当前选中的颜色（详见 CarbonUIColorPickWidget.h）。
 */
QColor CarbonUIColorPickWidget::color() const {
	return color_;
}

/**
 * @brief 设置当前颜色并同步全部子控件（详见 CarbonUIColorPickWidget.h）。
 */
void CarbonUIColorPickWidget::setColor(const QColor& color) {
	color_ = color.isValid() ? color : QColor(Qt::black);
	double hue = 0.0;
	double sat = 0.0;
	double val = 0.0;
	double alpha = 0.0;
	color_.getHsvF(&hue, &sat, &val, &alpha);
	area_->setHsv(hue, sat);
	slider_->setValue(val);
	syncEditors();
}

/**
 * @brief 装配子控件、布局与样式（详见 CarbonUIColorPickWidget.h）。
 */
void CarbonUIColorPickWidget::buildUi() {
	// 右列（含 Ok/Cancel 与预览块）已收紧到与选色区同高，面板高度即选色区 + 边距
	setFixedSize(kAreaWidth + kSliderWidth + kColumnWidth + 12 * 2 + 8 * 2, kAreaHeight + 10 * 2);

	auto* rootLayout = new QHBoxLayout(this);
	rootLayout->setContentsMargins(12, 10, 12, 10);
	rootLayout->setSpacing(8);

	// 选色区 + 明度滑条：取色回调统一交回本类，保持单一数据源
	area_ = new ColorAreaWidget(this);
	rootLayout->addWidget(area_);

	slider_ = new ValueSliderWidget(this);
	rootLayout->addWidget(slider_);

	area_->onPick = [this](double hue, double sat) {
		// 初始颜色为黑色（明度 0）时点选色区会"点哪儿都是黑"，自动提亮到满明度，
		// 保证第一次点选就能得到彩色；之后明度跟随滑条。
		const double val = slider_->value() > 0.0 ? slider_->value() : 1.0;
		applyHsv(hue, sat, val, true);
	};
	slider_->onPick = [this](double val) { applyHsv(area_->hue(), area_->sat(), val, true); };

	// 右列：Ok / Cancel、RGB、十六进制、透明度
	auto* column = new QVBoxLayout;
	// 右列整体高度收紧：列内容总高不得超过左侧选色区（230px），上下边缘对齐。
	// 22+22+24×6 控件 + 7×6 间隙 = 230px，恰好填满，全部间隙等宽（6px）。
	column->setSpacing(6);
	rootLayout->addLayout(column);

	okButton_ = new QPushButton(QStringLiteral("Ok"), this);
	okButton_->setObjectName(QLatin1String("carbonUIColorPickOk"));
	okButton_->setFixedSize(kColumnWidth, 22);
	column->addWidget(okButton_);

	cancelButton_ = new QPushButton(QStringLiteral("Cancel"), this);
	cancelButton_->setObjectName(QLatin1String("carbonUIColorPickCancel"));
	cancelButton_->setFixedSize(kColumnWidth, 22);
	column->addWidget(cancelButton_);

	const struct {
		const char* letter; ///< 通道字母（胶囊左端，左对齐）。
		const char* name;   ///< QSpinBox 的 objectName。
		int maxValue;       ///< 通道取值上限。
		QSpinBox** box;     ///< 成员指针的存放位置。
	} channels[] = {
	    {"R", "carbonUIColorPickSpinR", 255, &spinR_},
	    {"G", "carbonUIColorPickSpinG", 255, &spinG_},
	    {"B", "carbonUIColorPickSpinB", 255, &spinB_},
	};
	for (unsigned i = 0; i < sizeof(channels) / sizeof(channels[0]); ++i) {
		column->addWidget(makeChannelPill(channels[i].letter, channels[i].name,
		                                  channels[i].maxValue, this, channels[i].box));
		connect(*channels[i].box, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		        this, [this] { applyEditorChange(true); });
	}

	hexEdit_ = new QLineEdit(this);
	hexEdit_->setObjectName(QLatin1String("carbonUIColorPickHex"));
	hexEdit_->setAlignment(Qt::AlignRight);
	hexEdit_->setFixedSize(kColumnWidth, 24);
	column->addWidget(hexEdit_);
	connect(hexEdit_, &QLineEdit::editingFinished, this, [this] {
		QString text = hexEdit_->text().trimmed();
		if (!text.startsWith(QLatin1Char('#'))) {
			text.prepend(QLatin1Char('#'));
		}
		QColor parsed;
		parsed.setNamedColor(text);
		if (parsed.isValid()) {
			// 十六进制只含 RGB：保留当前透明度通道
			parsed.setAlpha(color_.alpha());
			setColor(parsed);
			emit colorChanged(color_);
		} else {
			syncEditors(); // 非法输入回滚为当前颜色的十六进制
		}
	});

	column->addWidget(makeChannelPill("A", "carbonUIColorPickSpinA", 100, this, &spinA_));
	connect(spinA_, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
	        [this] { applyEditorChange(true); });

	// 颜色预览块：跟随当前颜色实时填充（样式在 syncEditors 里随颜色刷新）
	previewLabel_ = new QLabel(this);
	previewLabel_->setObjectName(QLatin1String("carbonUIColorPickPreview"));
	previewLabel_->setFixedSize(kColumnWidth, 24);
	column->addWidget(previewLabel_);

	connect(okButton_, &QPushButton::clicked, this, [this] { emit accepted(); });
	connect(cancelButton_, &QPushButton::clicked, this, [this] { emit rejected(); });

	// CarbonUI 暗色面板：与其它 CarbonUIQ* 控件同一套面板色，强调色为主蓝
	QString style =
	    "CarbonUIColorPickWidget { background-color: #1f1f1f; border: 2px solid #0f0f0f;"
	    " border-radius: 9px; } "
	    "QWidget#carbonUIColorPickChannel { background-color: #2f2f2f; border: 1px solid #5f5f5f;"
	    " border-radius: 11px; } "
	    "QWidget#carbonUIColorPickChannel:hover { border-color: #777777; } "
	    "QWidget#carbonUIColorPickChannel QLabel { color: #cdcdcd; font-weight: bold; } "
	    "QSpinBox { background: transparent; color: #cdcdcd; border: none; } "
	    "QLineEdit { background-color: #2f2f2f; color: #cdcdcd; border: 1px solid #5f5f5f;"
	    " border-radius: 11px; padding: 2px 8px; } "
	    "QLineEdit:hover { color: #dddddd; border-color: #777777; } "
	    // 注意：不要写 min-height——QSS 的 min-height 按内容盒计算，叠加 padding 会把
	    // 控件实际高度撑过 setFixedSize 的定值，导致右列垂直间隙不均
	    "QPushButton { border-radius: 11px; font-weight: bold; } "
	    "QPushButton#carbonUIColorPickOk { background-color: #009ee8; color: white; border: none;"
	    " padding: 2px 14px; } "
	    "QPushButton#carbonUIColorPickOk:hover { background-color: #33b0ea; } "
	    "QPushButton#carbonUIColorPickCancel { background-color: #3c3c3c; color: #cdcdcd; border:"
	    " 1px solid #5f5f5f; padding: 2px 10px; } "
	    "QPushButton#carbonUIColorPickCancel:hover { color: #dddddd; border-color: #777777; }";
	setStyleSheet(style);
}

/**
 * @brief 把当前颜色回填到 RGB / 十六进制 / 透明度输入框（详见 CarbonUIColorPickWidget.h）。
 */
void CarbonUIColorPickWidget::syncEditors() {
	updating_ = true;
	spinR_->setValue(color_.red());
	spinG_->setValue(color_.green());
	spinB_->setValue(color_.blue());
	spinA_->setValue(percentFromAlpha(color_.alpha()));
	hexEdit_->setText(color_.name());
	updating_ = false;
	// 预览块背景随当前颜色刷新；带边框保证黑/白等极端色在深色面板上可见
	previewLabel_->setStyleSheet(
	    QStringLiteral("background-color: %1; border: 1px solid #5f5f5f; border-radius: 11px;")
	        .arg(color_.name()));
}

/**
 * @brief 选色区/滑条取色回调：按色相/饱和度/明度更新颜色（详见 CarbonUIColorPickWidget.h）。
 */
void CarbonUIColorPickWidget::applyHsv(double hue, double sat, double val, bool emitChange) {
	const int alpha = color_.alpha();
	color_.setHsvF(clamp01(hue), clamp01(sat), clamp01(val), alpha / 255.0);
	area_->setHsv(hue, sat);
	slider_->setValue(val);
	syncEditors();
	if (emitChange) {
		emit colorChanged(color_);
	}
}

/**
 * @brief RGB / 透明度输入回调：按输入更新颜色并同步选色区（详见 CarbonUIColorPickWidget.h）。
 */
void CarbonUIColorPickWidget::applyEditorChange(bool emitChange) {
	if (updating_) {
		return;
	}
	color_.setRgb(spinR_->value(), spinG_->value(), spinB_->value(),
	              alphaFromPercent(spinA_->value()));
	setColor(color_);
	if (emitChange) {
		emit colorChanged(color_);
	}
}
