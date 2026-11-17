/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIWidgetWithHdrBtn.cpp
 * @brief CarbonUIWidgetWithHdrBtn 的标题栏/内容区装配与按钮事件实现（接口见头文件）。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 标题栏与内容区用 0 边距的 QVBoxLayout 拼接；标题标签取 Expanding 水平策略，
 * 把后续加入的按钮自然推到右上角。每个按钮点击时捕获当时的 id，发出
 * @ref CarbonUIWidgetWithHdrBtn::buttonClicked 信号，避免调用方依赖按钮对象名。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIWidgetWithHdrBtn.h"

#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace {
/// @brief 标题栏高度（像素）。
constexpr int kHeaderHeight = 28;
/// @brief 图标按钮边长（像素）。
constexpr int kButtonSize = 24;
/// @brief 按钮图标边长（像素）。
constexpr int kIconSize = 16;
/// @brief 分割线高度（像素），比标题栏矮，两端留白。
constexpr int kDividerHeight = 16;
/// @brief 分割线宽度（像素）。
constexpr int kDividerWidth = 1;
/// @brief 默认边框宽度（像素），取自参考图的 2px 亮灰描边。
constexpr int kBorderWidth = 2;
/// @brief 默认边框颜色，取自参考图描边像素 #676767。
const char* const kBorderColor = "#676767";
} // namespace

/**
 * @brief 构造控件：应用深色 QSS -> 装配标题栏与内容区 -> 设置初始标题（详见头文件）。
 */
CarbonUIWidgetWithHdrBtn::CarbonUIWidgetWithHdrBtn(const QString& title, QWidget* parent)
    : QWidget(parent) {
	setObjectName(QStringLiteral("widgetWithHeaderButton"));
	// 边框由 paintEvent() 用 QPainter 直绘：QSS 的 border 在自定义 QWidget 子类上
	// 会被子控件覆盖，且依赖选择器时调用方改 objectName 还会导致规则失配。
	setStyleSheet(QStringLiteral(
	    "CarbonUIWidgetWithHdrBtn { background: #2f2f2f; }"
	    "#widgetWithHeaderButtonHeader { background: #3b3b3b; border-bottom: 1px solid #2b2b2b; }"
	    "#widgetWithHeaderTitle { color: #ffffff; font-size: 14px; padding-left: 8px; }"
	    "#widgetWithHeaderDivider { background: #242424; }"));

	buildUi();
	setTitle(title);
}

/**
 * @brief 自绘 2px #676767 默认边框（详见头文件）。
 * @param[in,out] event 待处理的绘制事件，不可为 nullptr。
 */
void CarbonUIWidgetWithHdrBtn::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	QPainter painter(this);
	const QColor borderColor{kBorderColor};
	QPen pen{borderColor};
	pen.setWidth(kBorderWidth);
	painter.setPen(pen);
	const int inset = kBorderWidth / 2;
	painter.drawRect(rect().adjusted(inset, inset, -inset, -inset));
}

/**
 * @brief 构建控件树：留出边框空间 + 固定高度标题栏 + 拉伸内容区。
 */
void CarbonUIWidgetWithHdrBtn::buildUi() {
	auto* root = new QVBoxLayout(this);
	// 给自绘边框留出空间，避免标题栏/内容区子控件盖住描边。
	root->setContentsMargins(kBorderWidth, kBorderWidth, kBorderWidth, kBorderWidth);
	root->setSpacing(0);

	auto* header = new QWidget(this);
	header->setObjectName(QStringLiteral("widgetWithHeaderButtonHeader"));
	header->setFixedHeight(kHeaderHeight);

	headerLayout_ = new QHBoxLayout(header);
	headerLayout_->setContentsMargins(0, 0, 4, 0);
	headerLayout_->setSpacing(2);

	titleLabel_ = new QLabel(header);
	titleLabel_->setObjectName(QStringLiteral("widgetWithHeaderTitle"));
	headerLayout_->addWidget(titleLabel_, 1);

	content_ = new QWidget(this);
	content_->setObjectName(QStringLiteral("widgetWithHeaderButtonContent"));

	root->addWidget(header);
	root->addWidget(content_, 1);
}

/**
 * @brief 设置左上角标题文本（详见头文件）。
 * @param[in] title 新的标题文本，可为空串。
 */
void CarbonUIWidgetWithHdrBtn::setTitle(const QString& title) {
	titleLabel_->setText(title);
}

/**
 * @brief 查询当前左上角标题文本（详见头文件）。
 * @return 当前标题文本。
 */
QString CarbonUIWidgetWithHdrBtn::title() const {
	return titleLabel_->text();
}

/**
 * @brief 追加图标按钮：非首按钮先加 1px 深色分割线，再加工按钮（详见头文件）。
 */
void CarbonUIWidgetWithHdrBtn::addButton(const QString& id, const QIcon& icon,
                                         const QString& toolTip) {
	if (id.isEmpty()) {
		return;
	}

	if (!buttons_.isEmpty()) {
		auto* divider = new QWidget(headerLayout_->parentWidget());
		divider->setObjectName(QStringLiteral("widgetWithHeaderDivider"));
		divider->setFixedSize(kDividerWidth, kDividerHeight);
		headerLayout_->addWidget(divider, 0, Qt::AlignVCenter);
	}

	auto* button = new QToolButton(headerLayout_->parentWidget());
	button->setObjectName(QStringLiteral("headerButton_") + id);
	button->setIcon(icon);
	button->setIconSize(QSize(kIconSize, kIconSize));
	button->setToolTip(toolTip);
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setFixedSize(kButtonSize, kButtonSize);
	button->setCursor(Qt::PointingHandCursor);
	button->setStyleSheet(
	    QStringLiteral("QToolButton { background: transparent; border: none; padding: 0px; }"
		               "QToolButton:hover { background: #4a4a4a; }"
		               "QToolButton:pressed { background: #5a5a5a; }"));
	headerLayout_->addWidget(button, 0, Qt::AlignVCenter);

	connect(button, &QToolButton::clicked, this, [this, id] { emit buttonClicked(id); });
	buttons_.insert(id, button);
}

/**
 * @brief 更新指定 id 按钮的悬停提示（详见头文件）。
 * @param[in] id      目标按钮标识，不存在时静默返回。
 * @param[in] toolTip 新的悬停提示。
 */
void CarbonUIWidgetWithHdrBtn::setButtonToolTip(const QString& id, const QString& toolTip) {
	auto it = buttons_.find(id);
	if (it != buttons_.end()) {
		it.value()->setToolTip(toolTip);
	}
}

/**
 * @brief 查询已添加的图标按钮数量（详见头文件）。
 * @return 标题栏右侧已添加的图标按钮数量。
 */
int CarbonUIWidgetWithHdrBtn::buttonCount() const {
	return buttons_.size();
}

/**
 * @brief 取得内容区容器（详见头文件）。
 * @return 内容区容器指针，非 nullptr。
 */
QWidget* CarbonUIWidgetWithHdrBtn::contentWidget() const {
	return content_;
}
