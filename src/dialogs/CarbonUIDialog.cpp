/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIDialog.cpp
 * @brief CarbonUIDialog 的装配与交互实现（外观契约见 CarbonUIDialog.h）。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 结构见 CarbonUIDialog.h 的控件树注释；这里只补充两个实现约定：
 * - 圆角靠 `WA_TranslucentBackground` + 面板子控件的 `border-radius` 实现，
 *   四角完全由 QSS 绘制，因此截图（grab）在四角是透明像素——视觉基线按
 *   ARGB32 保存，跨轮次比对稳定；
 * - 标题条拖拽只处理落在标题条内的左键，正文区的拖拽（选中文本等）不拦截。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIDialog.h"

#include <QtCore/QEvent>
#include <QtGui/QFont>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace
{
/// 标题条高度（像素），与参考界面一致。
constexpr int kHeaderHeight = 46;
/// 面板圆角半径（像素）。
constexpr int kCornerRadius = 9;
/// 对话框最小宽度（像素），保证标题与关闭按钮不挤在一起。
constexpr int kMinimumWidth = 520;
/// 标题条窗口按钮的边长（像素）。
constexpr int kWindowButtonSize = 30;

/// 最小化按钮字形（U+2014 EM DASH）。
const char* const kMinimizeGlyph = "\xE2\x80\x94";
/// 最大化按钮字形（U+25A1 WHITE SQUARE）。
const char* const kMaximizeGlyph = "\xE2\x96\xA1";
/// 最大化状态下的还原字形（U+2750 UPPER RIGHT DROP-SHADOWED WHITE SQUARE）。
const char* const kRestoreGlyph = "\xE2\x9D\x90";

/// 面板 QSS：正文底色 + 圆角 + 1px 黑边；最大化时圆角传 0。
QString frameStyle(int radius)
{
    return QStringLiteral("QWidget#CarbonUIDialogFrame { background-color: #212121;"
                          " border-radius: %1px; border: 1px solid #000000; }")
        .arg(radius);
}

/// 标题条 QSS：纯黑底 + 上两角圆角（下两角保持直角以贴合正文）。
QString headerStyle(int radius)
{
    return QStringLiteral("QWidget#CarbonUIDialogHeader { background-color: #0f0f0f;"
                          " border-top-left-radius: %1px; border-top-right-radius: %1px; }")
        .arg(radius);
}

/// 窗口按钮（最小化 / 最大化 / 关闭）QSS：透明底白字，悬停淡灰底。
QString windowButtonStyle(const QString& objectName)
{
    return QStringLiteral("QPushButton#%1 { background-color: transparent; color: #ffffff;"
                          " border: none; padding: 0px; }"
                          "QPushButton#%1:hover { background-color: #2a2a2a; border-radius: 6px; }")
        .arg(objectName);
}

/// 创建标题条窗口按钮：统一命名、样式、尺寸与鼠标形状。
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

/// 标题标签 QSS：白色加粗。
const char* const kTitleStyle = "color: #ffffff; background-color: transparent;";
/// 信息行 QSS：正文灰（与参考界面取样一致）。
const char* const kRowStyle = "color: #cdcdcd; background-color: transparent;";
} // namespace


/**
 * @brief 构造对话框：装配控件树并设置无边框圆角窗口（详见 CarbonUIDialog.h）。
 */
CarbonUIDialog::CarbonUIDialog(const QString& title, QWidget* parent)
    : QDialog(parent),
      m_frame(nullptr),
      m_header(nullptr),
      m_titleLabel(nullptr),
      m_minimizeButton(nullptr),
      m_maximizeButton(nullptr),
      m_closeButton(nullptr),
      m_body(nullptr),
      m_bodyLayout(nullptr),
      m_dragging(false)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(QStringLiteral("CarbonUIDialog { background: transparent; }"));
    setMinimumWidth(kMinimumWidth);

    buildUi();
    setTitleText(title);
}

/**
 * @brief 装配面板 / 标题条 / 正文区三层控件树（仅构造时调用）。
 */
void CarbonUIDialog::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_frame = new QWidget(this);
    m_frame->setObjectName(QStringLiteral("CarbonUIDialogFrame"));
    m_frame->setStyleSheet(frameStyle(kCornerRadius));
    rootLayout->addWidget(m_frame);

    auto* frameLayout = new QVBoxLayout(m_frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);

    // 标题条：左标题 + 弹性空间 + 右窗口按钮（最小化 / 最大化 / 关闭）
    m_header = new QWidget(m_frame);
    m_header->setObjectName(QStringLiteral("CarbonUIDialogHeader"));
    m_header->setStyleSheet(headerStyle(kCornerRadius));
    m_header->setFixedHeight(kHeaderHeight);
    auto* headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(26, 0, 14, 0);
    headerLayout->setSpacing(8);

    m_titleLabel = new QLabel(m_header);
    m_titleLabel->setObjectName(QStringLiteral("CarbonUIDialogTitle"));
    m_titleLabel->setStyleSheet(QLatin1String(kTitleStyle));
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    m_titleLabel->setFont(titleFont);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);

    // 最小化 / 最大化默认隐藏：调用方用 setMinimize/MinimizeButtonVisible 单独打开
    m_minimizeButton =
        createWindowButton(m_header, QStringLiteral("CarbonUIDialogMinimize"),
                           QString::fromUtf8(kMinimizeGlyph));
    m_minimizeButton->hide();
    headerLayout->addWidget(m_minimizeButton, 0, Qt::AlignVCenter);
    QObject::connect(m_minimizeButton, &QPushButton::clicked, this, &QDialog::showMinimized);

    m_maximizeButton =
        createWindowButton(m_header, QStringLiteral("CarbonUIDialogMaximize"),
                           QString::fromUtf8(kMaximizeGlyph));
    m_maximizeButton->hide();
    headerLayout->addWidget(m_maximizeButton, 0, Qt::AlignVCenter);
    QObject::connect(m_maximizeButton, &QPushButton::clicked, this, [this] {
        if (isMaximized())
        {
            showNormal();
        }
        else
        {
            showMaximized();
        }
    });

    m_closeButton =
        createWindowButton(m_header, QStringLiteral("CarbonUIDialogClose"),
                           QString::fromUtf8("\xE2\x9C\x95"));
    headerLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);
    QObject::connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    frameLayout->addWidget(m_header);

    // 正文区：逐行追加 "名称: 取值"
    m_body = new QWidget(m_frame);
    m_body->setObjectName(QStringLiteral("CarbonUIDialogBody"));
    // 纵向节奏对齐参考界面：正文首行距标题条 26px，行与行之间 32px，底部留白 32px
    m_bodyLayout = new QVBoxLayout(m_body);
    m_bodyLayout->setContentsMargins(26, 26, 26, 32);
    m_bodyLayout->setSpacing(32);
    frameLayout->addWidget(m_body, 1);
}

/**
 * @brief 更新标题文字（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::setTitleText(const QString& title)
{
    if (m_titleLabel != nullptr)
    {
        m_titleLabel->setText(title);
    }
}

/**
 * @brief 查询标题文字（详见 CarbonUIDialog.h）。
 * @return 标题标签当前文本。
 */
QString CarbonUIDialog::titleText() const
{
    return m_titleLabel != nullptr ? m_titleLabel->text() : QString();
}

/**
 * @brief 显示或隐藏最小化按钮（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::setMinimizeButtonVisible(bool visible)
{
    if (m_minimizeButton != nullptr)
    {
        m_minimizeButton->setVisible(visible);
    }
}

/**
 * @brief 查询最小化按钮是否显示（详见 CarbonUIDialog.h）。
 */
bool CarbonUIDialog::isMinimizeButtonVisible() const
{
    return m_minimizeButton != nullptr && m_minimizeButton->isVisibleTo(this);
}

/**
 * @brief 显示或隐藏最大化按钮（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::setMaximizeButtonVisible(bool visible)
{
    if (m_maximizeButton != nullptr)
    {
        m_maximizeButton->setVisible(visible);
    }
}

/**
 * @brief 查询最大化按钮是否显示（详见 CarbonUIDialog.h）。
 */
bool CarbonUIDialog::isMaximizeButtonVisible() const
{
    return m_maximizeButton != nullptr && m_maximizeButton->isVisibleTo(this);
}

/**
 * @brief 按窗口状态刷新面板圆角与最大化字形（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::syncWindowState()
{
    const bool maximized = isMaximized();
    const int radius = maximized ? 0 : kCornerRadius;

    if (m_frame != nullptr)
    {
        m_frame->setStyleSheet(frameStyle(radius));
    }
    if (m_header != nullptr)
    {
        m_header->setStyleSheet(headerStyle(radius));
    }
    if (m_maximizeButton != nullptr)
    {
        m_maximizeButton->setText(QString::fromUtf8(maximized ? kRestoreGlyph : kMaximizeGlyph));
    }
}

/**
 * @brief 窗口状态变化时同步外观（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::changeEvent(QEvent* event)
{
    if (event != nullptr && event->type() == QEvent::WindowStateChange)
    {
        syncWindowState();
    }
    QDialog::changeEvent(event);
}

/**
 * @brief 追加 "名称: 取值" 信息行（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::addInfoRow(const QString& label, const QString& value)
{
    addInfoRow(QStringLiteral("%1: %2").arg(label, value));
}

/**
 * @brief 追加原样文本信息行（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::addInfoRow(const QString& text)
{
    if (m_bodyLayout == nullptr)
    {
        return;
    }

    auto* row = new QLabel(text, m_body);
    row->setObjectName(QStringLiteral("CarbonUIDialogRow"));
    row->setStyleSheet(QLatin1String(kRowStyle));
    QFont rowFont = row->font();
    rowFont.setPointSize(10);
    row->setFont(rowFont);
    m_bodyLayout->addWidget(row);
}

/**
 * @brief 记录标题条拖拽起点（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::mousePressEvent(QMouseEvent* event)
{
    // 最大化时窗口已铺满可用区域，不再响应标题条拖拽
    if (event != nullptr && event->button() == Qt::LeftButton && m_header != nullptr
        && !isMaximized() && m_header->geometry().contains(event->pos()))
    {
        m_dragging = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

/**
 * @brief 拖拽移动窗口（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && event != nullptr)
    {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

/**
 * @brief 结束拖拽（详见 CarbonUIDialog.h）。
 */
void CarbonUIDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}
