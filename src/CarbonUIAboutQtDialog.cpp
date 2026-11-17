/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIAboutQtDialog.cpp
 * @brief CarbonUIAboutQtDialog 的装配与交互实现（外观契约见 CarbonUIAboutQtDialog.h）。
 * @author CarbonUI
 * @date  2026-11-17
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 结构见 CarbonUIAboutQtDialog.h 的控件树注释；实现约定与 CarbonUIDialog 一致：
 * - 圆角靠 `WA_TranslucentBackground` + 面板子控件的 `border-radius` 实现；
 * - 标题条拖拽只处理落在标题条内的左键，正文区拖拽（选中文本）不拦截；
 * - 全部样式经本文件内的 QSS 常量就地生效，调用方零设置。
 *
 * 与 CarbonUIDialog 的差异点：正文是"徽标行 + 富文本段落"而非键值行，
 * 因此不继承 CarbonUIDialog（其正文 API 只支持纯文本行）而独立装配 QDialog。
 *
 * 线程约定：只在 GUI 主线程使用。
 */

#include "CarbonUIAboutQtDialog.h"

#include <QtCore/QDate>
#include <QtCore/QEvent>
#include <QtGui/QFont>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace
{
/// 标题条高度（像素），与 CarbonUIDialog / 参考界面一致。
constexpr int kHeaderHeight = 46;
/// 面板圆角半径（像素）。
constexpr int kCornerRadius = 9;
/// 对话框最小宽度（像素），保证段落排版与标题/关闭按钮不挤在一起。
constexpr int kMinimumWidth = 520;
/// 标题条关闭按钮的边长（像素）。
constexpr int kWindowButtonSize = 30;
/// Qt 徽标显示边长（像素）；实际绘制 2 倍尺寸并设 DevicePixelRatio 保证清晰。
constexpr int kLogoSize = 48;
/// 正文文字色（与 CarbonUIDialog 信息行取样一致）。
const char* const kBodyColor = "#cdcdcd";
/// 超链接颜色（参考图中的亮蓝）。
const char* const kLinkColor = "#4FA3FF";
/// 正文标签 objectName：宿主测试与样式查找的唯一锚点。
const char* const kBodyObjectName = "CarbonUIAboutQtText";

/// 面板 QSS：正文底色 + 圆角 + 1px 黑边。
const char* const kFrameStyle = "QWidget#CarbonUIDialogFrame { background-color: #212121;"
                                " border-radius: 9px; border: 1px solid #000000; }";
/// 标题条 QSS：纯黑底 + 上两角圆角（下两角保持直角以贴合正文）。
const char* const kHeaderStyle =
    "QWidget#CarbonUIDialogHeader { background-color: #0f0f0f;"
    " border-top-left-radius: 9px; border-top-right-radius: 9px; }";
/// 关闭按钮 QSS：透明底白字，悬停淡灰底。
const char* const kCloseButtonStyle =
    "QPushButton#CarbonUIDialogClose { background-color: transparent; color: #ffffff;"
    " border: none; padding: 0px; }"
    "QPushButton#CarbonUIDialogClose:hover { background-color: #2a2a2a; border-radius: 6px; }";
/// 标题标签 QSS：白色加粗。
const char* const kTitleStyle = "color: #ffffff; background-color: transparent;";
/// 正文标签 QSS：次级灰（ID 选择器必须带 # 前缀，否则永远不命中导致
/// 回落默认黑色文字、深底上不可见），链接色在富文本内联样式里逐链接指定。
const char* const kBodyLabelStyle = "QLabel#CarbonUIAboutQtText { color: #cdcdcd;"
                                    " background-color: transparent; }";

/// 关闭按钮字形（U+2715 MULTIPLICATION X）。
const char* const kCloseGlyph = "\xE2\x9C\x95";

/**
 * @brief 把裸 URL 包成内联着色的超链接片段。
 * @param[in] url 目标地址，同时作为链接显示文字。
 * @return 富文本 `<a>` 片段，颜色取 @ref kLinkColor、无下划线。
 */
QString styledLink(const QString& url)
{
    return QStringLiteral("<a href=\"%1\" style=\"color:%2; text-decoration:none;\">%1</a>")
        .arg(url, QLatin1String(kLinkColor));
}

/**
 * @brief 拼装「关于 Qt」富文本正文（段落措辞对齐 Qt 官方对话框）。
 * @return HTML 字符串；运行时 Qt 版本经 qVersion() 填充，版权年份取当前年份。
 */
QString aboutQtHtml()
{
    const QString version = QString::fromLatin1(qVersion());
    const QString year = QString::number(QDate::currentDate().year());
    const QString downloadLink = styledLink(QStringLiteral("https://download.qt.io"));
    const QString licensingLink = styledLink(QStringLiteral("https://www.qt.io/licensing/"));
    const QString qtIoLink = styledLink(QStringLiteral("https://www.qt.io"));

    QString html = QStringLiteral(
                       "<p>Qt software is an application service framework software.</p>"
                       "<p>Qt is available under different licensing options designed to "
                       "accommodate the needs of our various users:</p>"
                       "<p>Qt licensed under commercial licenses is appropriate for development "
                       "of proprietary/commercial software where you do not want to share any "
                       "source code with third parties or otherwise cannot comply with the terms "
                       "of the GNU LGPL version 3.</p>"
                       "<p>Qt licensed under the GNU Lesser General Public License (LGPL) "
                       "version 3 is appropriate for the development of Qt applications provided "
                       "you can comply with the terms and conditions of the GNU LGPL version 3 "
                       "(or GNU GPL version 3).</p>"
                       "<p>The application created using Qt version %1.<br/>"
                       "Qt %1 can be found here %2.<br/>"
                       "See %3 for an overview of Qt licensing.</p>"
                       "<p>Copyright (C) %4 The Qt Company Ltd.<br/>"
                       "Qt and the Qt logo are trademarks of The Qt Company Ltd.<br/>"
                       "See %5 for more information.</p>")
                       .arg(version, downloadLink, licensingLink, year, qtIoLink);
    return html;
}

/**
 * @brief 程序化绘制 Qt 徽标：绿色圆角方块 + 白色加粗 "Qt"。
 * @return 2 倍尺寸绘制的 QPixmap（DevicePixelRatio=2），显示为 @ref kLogoSize 见方。
 */
QPixmap createQtLogo()
{
    const int physical = kLogoSize * 2;
    QPixmap pixmap(physical, physical);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x41, 0xcd, 0x52));
    painter.drawRoundedRect(0, 0, physical, physical, 18, 18);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(physical / 2);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QStringLiteral("Qt"));
    painter.end();

    pixmap.setDevicePixelRatio(2.0);
    return pixmap;
}
} // namespace

/**
 * @brief 构造对话框：装配控件树并设置无边框圆角窗口（详见 CarbonUIAboutQtDialog.h）。
 */
CarbonUIAboutQtDialog::CarbonUIAboutQtDialog(QWidget* parent)
    : QDialog(parent),
      m_frame(nullptr),
      m_header(nullptr),
      m_titleLabel(nullptr),
      m_closeButton(nullptr),
      m_dragging(false)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(QStringLiteral("CarbonUIAboutQtDialog { background: transparent; }"));
    setMinimumWidth(kMinimumWidth);

    buildUi();
}

/**
 * @brief 装配面板 / 标题条 / 徽标与富文本正文（仅构造时调用）。
 */
void CarbonUIAboutQtDialog::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_frame = new QWidget(this);
    m_frame->setObjectName(QStringLiteral("CarbonUIDialogFrame"));
    m_frame->setStyleSheet(QLatin1String(kFrameStyle));
    rootLayout->addWidget(m_frame);

    auto* frameLayout = new QVBoxLayout(m_frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);

    // 标题条：左标题 + 弹性空间 + 右关闭按钮
    m_header = new QWidget(m_frame);
    m_header->setObjectName(QStringLiteral("CarbonUIDialogHeader"));
    m_header->setStyleSheet(QLatin1String(kHeaderStyle));
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
    m_titleLabel->setText(QStringLiteral("ABOUT QT"));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);

    m_closeButton = new QPushButton(QString::fromUtf8(kCloseGlyph), m_header);
    m_closeButton->setObjectName(QStringLiteral("CarbonUIDialogClose"));
    m_closeButton->setStyleSheet(QLatin1String(kCloseButtonStyle));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFlat(true);
    m_closeButton->setFixedSize(kWindowButtonSize, kWindowButtonSize);
    headerLayout->addWidget(m_closeButton, 0, Qt::AlignVCenter);
    QObject::connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    frameLayout->addWidget(m_header);

    // 正文区：徽标行 + 富文本段落
    auto* body = new QWidget(m_frame);
    body->setObjectName(QStringLiteral("CarbonUIAboutQtBody"));
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(26, 26, 26, 32);
    bodyLayout->setSpacing(18);
    frameLayout->addWidget(body, 1);

    auto* logoRow = new QWidget(body);
    auto* logoLayout = new QHBoxLayout(logoRow);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(14);

    auto* logoLabel = new QLabel(logoRow);
    logoLabel->setPixmap(createQtLogo());
    logoLabel->setFixedSize(kLogoSize, kLogoSize);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLayout->addWidget(logoLabel, 0, Qt::AlignVCenter);

    auto* headingLabel = new QLabel(QStringLiteral("About Qt"), logoRow);
    QFont headingFont = headingLabel->font();
    headingFont.setBold(true);
    headingFont.setPointSize(14);
    headingLabel->setFont(headingFont);
    headingLabel->setStyleSheet(QStringLiteral("color: %1; background-color: transparent;")
                                    .arg(QLatin1String(kBodyColor)));
    logoLayout->addWidget(headingLabel, 0, Qt::AlignVCenter);
    logoLayout->addStretch(1);
    bodyLayout->addWidget(logoRow);

    auto* textLabel = new QLabel(body);
    textLabel->setObjectName(QLatin1String(kBodyObjectName));
    textLabel->setStyleSheet(QLatin1String(kBodyLabelStyle));
    QFont textFont = textLabel->font();
    textFont.setPointSize(10);
    textLabel->setFont(textFont);
    textLabel->setTextFormat(Qt::RichText);
    textLabel->setText(aboutQtHtml());
    textLabel->setWordWrap(true);
    textLabel->setOpenExternalLinks(true);
    textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByMouse);
    bodyLayout->addWidget(textLabel);
    bodyLayout->addStretch(1);
}

/**
 * @brief 一行代码弹出模态「关于 Qt」对话框（详见 CarbonUIAboutQtDialog.h）。
 */
void CarbonUIAboutQtDialog::showAboutQt(QWidget* parent)
{
    CarbonUIAboutQtDialog dialog(parent);
    dialog.exec();
}

/**
 * @brief 记录标题条拖拽起点（详见 CarbonUIAboutQtDialog.h）。
 */
void CarbonUIAboutQtDialog::mousePressEvent(QMouseEvent* event)
{
    if (event != nullptr && event->button() == Qt::LeftButton && m_header != nullptr
        && m_header->geometry().contains(event->pos()))
    {
        m_dragging = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QDialog::mousePressEvent(event);
}

/**
 * @brief 拖拽移动窗口（详见 CarbonUIAboutQtDialog.h）。
 */
void CarbonUIAboutQtDialog::mouseMoveEvent(QMouseEvent* event)
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
 * @brief 结束拖拽（详见 CarbonUIAboutQtDialog.h）。
 */
void CarbonUIAboutQtDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}
