/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTitleBar.cpp
 * @brief CarbonUIQTitleBar 的装配与交互实现（外观契约见 CarbonUIQTitleBar.h）。
 * @author CarbonUI
 * @date  2026-11-24
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 结构与交互约定见头文件；这里补充两个实现要点：
 * - 尺寸约定与 CarbonUIQMenuBar 的 26px 菜单栏等高，两者在
 *   QMainWindow::setMenuWidget() 的宿主里上下拼接即成完整顶栏；
 * - 拖拽 / 双击的目标都是所在顶层窗口（`window()`），本控件只管自己的横条，
 *   不负责窗口圆角 mask 与边框绘制——那是宿主窗口 `paintEvent()` 的职责。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIQTitleBar.h"

#include <QtCore/QEvent>
#include <QtGui/QFont>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

namespace {

/// 标题条高度（像素），与 CarbonUIQMenuBar 的 26px 菜单栏等高。
constexpr int kTitleBarHeight = 26;
/// 标题条窗口按钮边长（像素），须小于标题条高度以留出上下留白。
constexpr int kWindowButtonSize = 24;
/// 标题条最左侧应用 logo 边长（像素）。
constexpr int kTitleLogoSize = 18;

/// 最小化按钮字形（U+2014 EM DASH）。
const char* const kMinimizeGlyph = "\xE2\x80\x94";
/// 最大化按钮字形（U+25A1 WHITE SQUARE）。
const char* const kMaximizeGlyph = "\xE2\x96\xA1";
/// 最大化状态下的还原字形（U+2750 UPPER RIGHT DROP-SHADOWED WHITE SQUARE）。
const char* const kRestoreGlyph = "\xE2\x9D\x90";
/// 关闭按钮字形（U+2715 MULTIPLICATION X）。
const char* const kCloseGlyph = "\xE2\x9C\x95";

/// 标题条 QSS：纯黑底 + 上两角圆角；最大化时圆角传 0。
QString titleBarStyle(int radius)
{
	return QStringLiteral("QWidget#CarbonUIQTitleBar { background-color: #0f0f0f;"
	                      " border-top-left-radius: %1px; border-top-right-radius: %1px; }")
	    .arg(radius);
}

/// 标题条窗口按钮 QSS：透明底白字，悬停淡灰底。
QString windowButtonStyle(const QString& objectName)
{
	return QStringLiteral("QPushButton#%1 { background-color: transparent; color: #ffffff;"
	                      " border: none; padding: 0px; }"
	                      "QPushButton#%1:hover { background-color: #2a2a2a; border-radius: 6px; }")
	    .arg(objectName);
}

/// 创建一枚标题条窗口按钮：统一命名、样式、尺寸与鼠标形状。
QPushButton* createWindowButton(QWidget* parent, const QString& objectName, const QString& glyph)
{
	auto* button = new QPushButton(glyph, parent);
	button->setObjectName(objectName);
	button->setStyleSheet(windowButtonStyle(objectName));
	button->setCursor(Qt::PointingHandCursor);
	button->setFlat(true);
	button->setFixedSize(kWindowButtonSize, kWindowButtonSize);
	return button;
}

} // namespace

/**
 * @brief 构造标题条并完成外观装配（详见 CarbonUIQTitleBar.h）。
 */
CarbonUIQTitleBar::CarbonUIQTitleBar(QWidget* parent) : QWidget(parent) {
	buildUi();
	attachWindow();
	syncToWindowState();
}

/**
 * @brief 装配标题条控件树（仅构造时调用）。
 */
void CarbonUIQTitleBar::buildUi() {
	setObjectName(QStringLiteral("CarbonUIQTitleBar"));
	setFixedHeight(kTitleBarHeight);
	setStyleSheet(titleBarStyle(m_cornerRadius));

	auto* headerLayout = new QHBoxLayout(this);
	headerLayout->setContentsMargins(10, 0, 14, 0);
	headerLayout->setSpacing(8);

	// 最左侧 logo：等比缩放到 18px，透明底渲染
	m_logoLabel = new QLabel(this);
	m_logoLabel->setObjectName(QStringLiteral("CarbonUIQTitleBarLogo"));
	m_logoLabel->setAlignment(Qt::AlignCenter);
	m_logoLabel->setFixedSize(kTitleLogoSize, kTitleLogoSize);
	m_logoLabel->setStyleSheet(QStringLiteral("background-color: transparent;"));
	headerLayout->addWidget(m_logoLabel);

	// 白色加粗标题，占满剩余宽度
	m_titleLabel = new QLabel(this);
	m_titleLabel->setObjectName(QStringLiteral("CarbonUIQTitleBarTitle"));
	m_titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; background-color: transparent;"));
	QFont titleFont = m_titleLabel->font();
	titleFont.setBold(true);
	titleFont.setPointSize(11);
	titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
	m_titleLabel->setFont(titleFont);
	m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	headerLayout->addWidget(m_titleLabel);
	headerLayout->addStretch(1);

	// 右侧窗口按钮：最小化 → 最大化 → 关闭
	m_minimizeButton = createWindowButton(this, QStringLiteral("CarbonUIQTitleBarMinimize"),
	                                      QString::fromUtf8(kMinimizeGlyph));
	headerLayout->addWidget(m_minimizeButton, 0, Qt::AlignVCenter);
	QObject::connect(m_minimizeButton, &QPushButton::clicked, this, [this] {
		if (m_boundWindow != nullptr) {
			m_boundWindow->showMinimized();
		}
	});

	m_maximizeButton = createWindowButton(this, QStringLiteral("CarbonUIQTitleBarMaximize"),
	                                      QString::fromUtf8(kMaximizeGlyph));
	headerLayout->addWidget(m_maximizeButton, 0, Qt::AlignVCenter);
	QObject::connect(m_maximizeButton, &QPushButton::clicked, this, [this] {
		if (m_boundWindow == nullptr) {
			return;
		}
		if (m_boundWindow->isMaximized()) {
			m_boundWindow->showNormal();
		} else {
			m_boundWindow->showMaximized();
		}
	});

	m_closeButton = createWindowButton(this, QStringLiteral("CarbonUIQTitleBarClose"),
	                                   QString::fromUtf8(kCloseGlyph));
	headerLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);
	QObject::connect(m_closeButton, &QPushButton::clicked, this, [this] {
		if (m_boundWindow != nullptr) {
			m_boundWindow->close();
		}
	});
}

/**
 * @brief 绑定所在顶层窗口（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::attachWindow() {
	QWidget* boundWindow = window();
	// 尚未进入任何顶层窗口（window() 返回自身）时不绑定，等 showEvent 再试。
	if (boundWindow == this || m_boundWindow == boundWindow) {
		return;
	}
	if (m_boundWindow != nullptr) {
		m_boundWindow->removeEventFilter(this);
		m_boundWindow->disconnect(this);
	}
	m_boundWindow = boundWindow;
	if (m_boundWindow == nullptr) {
		return;
	}
	// 标题文字跟随窗口标题；状态变化走事件过滤器自动刷新字形与圆角。
	QObject::connect(m_boundWindow, &QWidget::windowTitleChanged, this,
	                 &CarbonUIQTitleBar::setTitleText);
	m_boundWindow->installEventFilter(this);
	setTitleText(m_boundWindow->windowTitle());
	syncToWindowState();
}

/**
 * @brief 更新标题文字（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::setTitleText(const QString& text) {
	if (m_titleLabel != nullptr) {
		m_titleLabel->setText(text);
	}
}

/**
 * @brief 查询标题文字（详见 CarbonUIQTitleBar.h）。
 */
QString CarbonUIQTitleBar::titleText() const {
	return m_titleLabel != nullptr ? m_titleLabel->text() : QString();
}

/**
 * @brief 设置标题条最左侧 logo（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::setLogoPixmap(const QPixmap& logo) {
	if (m_logoLabel == nullptr) {
		return;
	}
	if (logo.isNull()) {
		m_logoLabel->clear();
		return;
	}
	m_logoLabel->setPixmap(logo.scaled(kTitleLogoSize, kTitleLogoSize, Qt::KeepAspectRatio,
	                                   Qt::SmoothTransformation));
}

/**
 * @brief 告知所在窗口圆角半径（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::setCornerRadius(int radius) {
	m_cornerRadius = radius < 0 ? 0 : radius;
	syncToWindowState();
}

/**
 * @brief 按所在窗口状态刷新字形与圆角（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::syncToWindowState() {
	const bool maximized = m_boundWindow != nullptr && m_boundWindow->isMaximized();
	const int radius = maximized ? 0 : m_cornerRadius;
	setStyleSheet(titleBarStyle(radius));
	if (m_maximizeButton != nullptr) {
		m_maximizeButton->setText(QString::fromUtf8(maximized ? kRestoreGlyph : kMaximizeGlyph));
	}
}

/**
 * @brief 记录标题条拖拽起点（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::mousePressEvent(QMouseEvent* event) {
	// 最大化时窗口已铺满工作区，不再响应标题条拖拽
	if (event != nullptr && event->button() == Qt::LeftButton && m_boundWindow != nullptr
	    && !m_boundWindow->isMaximized()
	    && qobject_cast<QPushButton*>(childAt(event->pos())) == nullptr) {
		m_dragging = true;
		m_dragOffset = event->globalPos() - m_boundWindow->frameGeometry().topLeft();
		event->accept();
		return;
	}
	QWidget::mousePressEvent(event);
}

/**
 * @brief 拖拽移动所在窗口（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::mouseMoveEvent(QMouseEvent* event) {
	if (m_dragging && event != nullptr && m_boundWindow != nullptr) {
		m_boundWindow->move(event->globalPos() - m_dragOffset);
		event->accept();
		return;
	}
	QWidget::mouseMoveEvent(event);
}

/**
 * @brief 结束拖拽（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::mouseReleaseEvent(QMouseEvent* event) {
	m_dragging = false;
	QWidget::mouseReleaseEvent(event);
}

/**
 * @brief 双击标题条切换最大化 / 还原（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event != nullptr && event->button() == Qt::LeftButton && m_boundWindow != nullptr
	    && qobject_cast<QPushButton*>(childAt(event->pos())) == nullptr) {
		m_dragging = false; // 双击的第二下按下可能已开始拖拽，切换前先终止。
		if (m_boundWindow->isMaximized()) {
			m_boundWindow->showNormal();
		} else {
			m_boundWindow->showMaximized();
		}
		event->accept();
		return;
	}
	QWidget::mouseDoubleClickEvent(event);
}

/**
 * @brief 所在窗口状态变化时自动刷新（详见 CarbonUIQTitleBar.h）。
 */
bool CarbonUIQTitleBar::eventFilter(QObject* watched, QEvent* event) {
	if (watched == m_boundWindow && event != nullptr
	    && event->type() == QEvent::WindowStateChange) {
		syncToWindowState();
	}
	return QWidget::eventFilter(watched, event);
}

/**
 * @brief 进入顶层窗口链后补绑定所在窗口（详见 CarbonUIQTitleBar.h）。
 */
void CarbonUIQTitleBar::showEvent(QShowEvent* event) {
	attachWindow();
	QWidget::showEvent(event);
}
