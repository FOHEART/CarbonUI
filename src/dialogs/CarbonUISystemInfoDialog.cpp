/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUISystemInfoDialog.cpp
 * @brief CarbonUISystemInfoDialog 的实现：正文装配、双语切换与剪贴板复制。
 * @ingroup carbonui
 *
 * @details
 * 实现 CarbonUISystemInfoDialog.h。构造时先完成 CarbonUIDialog 外壳内的
 * 正文装配（分组框 + 只读 QTextEdit + 按钮行），再同步采集系统信息并按
 * 初始语言渲染。setLanguage() 只重建文案（重新渲染缓存数据），外部命令
 * 采集只在构造时发生一次。滚动条样式必须刷在 QScrollBar 对象自身
 * （祖先样式表会被控件自身样式覆盖）。
 */

#include "CarbonUISystemInfoDialog.h"

#include <QtGui/QClipboard>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtCore/QLocale>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include "CarbonUIQPushButton.h"
#include "CarbonUIQScrollBar.h"

CarbonUISystemInfoDialog::CarbonUISystemInfoDialog(QWidget* parent)
    : CarbonUIDialog(QStringLiteral("System Information"), parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    buildBody();

    // 构造时同步采集一次（含带超时的外部命令）；语言切换只重渲染。
    m_data = CarbonUISystemInfo::collectSystemInfo();

    // 初始语言：与 CarbonUIUsageStatusBar 一致，按系统区域初始化；
    // 宿主若使用自己的语言配置（如 LanguageManager），在创建后显式调用
    // setLanguage() 覆盖一次即可。
    setLanguage(QLocale::system().name());

    connect(m_copyButton, &QPushButton::clicked,
            this, &CarbonUISystemInfoDialog::copyToClipboard);
    connect(m_okButton, &QPushButton::clicked,
            this, &CarbonUISystemInfoDialog::requestClose);
}

void CarbonUISystemInfoDialog::buildBody()
{
    // CarbonUIDialog 外壳的正文容器：装载分组框与按钮行。
    QWidget* body = findChild<QWidget*>(QStringLiteral("CarbonUIDialogBody"));
    if (body == nullptr)
    {
        return;
    }

    // 内容区统一背景色（透明，由外壳深色面板透出）与白色字体，与
    // 「关于软件」弹窗一致；去掉分组框 / 文本浏览框的亮色默认边框。
    QWidget* content = new QWidget(body);
    content->setStyleSheet(QStringLiteral(
        "QWidget { background: transparent; color: #ffffff;"
        " font-family: 'Microsoft YaHei'; font-size: 11pt; }"
        "QGroupBox { background: transparent; color: #ffffff; border: none; }"
        "QGroupBox::title { color: #ffffff; }"
        "QTextEdit { background: transparent; color: #ffffff;"
        " border: none; padding: 0px; }"));

    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    m_groupBox = new QGroupBox(content);
    // 标题居中（与原 .ui 一致）；正文顶部留出标题绘制区，避免首行与标题重叠。
    m_groupBox->setAlignment(Qt::AlignHCenter);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_groupBox);
    groupLayout->setContentsMargins(0, 22, 0, 0);
    groupLayout->setSpacing(0);

    m_infoText = new QTextEdit(m_groupBox);
    m_infoText->setReadOnly(true);
    groupLayout->addWidget(m_infoText, 1);
    contentLayout->addWidget(m_groupBox, 1);

    // 底部按钮行：占位弹簧 + 复制 + 确定。
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    m_copyButton = new QPushButton(content);
    m_okButton = new QPushButton(content);
    buttonLayout->addWidget(m_copyButton);
    buttonLayout->addWidget(m_okButton);
    contentLayout->addLayout(buttonLayout);

    CarbonUIQPushButton::applyStyle(m_copyButton);
    CarbonUIQPushButton::applyStyle(m_okButton);
    // 滚动条样式必须刷在 QScrollBar 对象自身（祖先样式表会被控件自身样式覆盖）。
    CarbonUIQScrollBar::applyStyle(m_infoText->verticalScrollBar());
    CarbonUIQScrollBar::applyStyle(m_infoText->horizontalScrollBar());

    QVBoxLayout* bodyLayout = qobject_cast<QVBoxLayout*>(body->layout());
    if (bodyLayout != nullptr)
    {
        bodyLayout->setContentsMargins(12, 12, 12, 12);
        bodyLayout->setSpacing(12);
        bodyLayout->addWidget(content, 1);
    }
    else
    {
        // 外壳布局不是 QVBoxLayout 时兜底：直接把内容挂到 body 上。
        QVBoxLayout* fallback = new QVBoxLayout(body);
        fallback->setContentsMargins(12, 12, 12, 12);
        fallback->addWidget(content, 1);
    }
}

void CarbonUISystemInfoDialog::setLanguage(const QString& languageCode)
{
    m_language = languageCode;
    m_labels = CarbonUISystemInfo::labelsForLanguage(languageCode);
    retranslateUi();
    refreshInfoText();
}

void CarbonUISystemInfoDialog::copyToClipboard()
{
    QApplication::clipboard()->setText(m_infoText->toPlainText());
}

void CarbonUISystemInfoDialog::requestClose()
{
    close();
}

void CarbonUISystemInfoDialog::retranslateUi()
{
    // 标题 / 分组框 / 按钮统一按当前语言重建。
    const QString title = CarbonUISystemInfo::isChineseLanguageCode(m_language)
        ? QStringLiteral("系统信息") : QStringLiteral("System Information");
    setTitleText(title);
    setWindowTitle(title);
    m_groupBox->setTitle(title);
    m_copyButton->setText(CarbonUISystemInfo::isChineseLanguageCode(m_language)
        ? QStringLiteral("复制信息") : QStringLiteral("Copy to Clipboard"));
    m_okButton->setText(CarbonUISystemInfo::isChineseLanguageCode(m_language)
        ? QStringLiteral("确定") : QStringLiteral("OK"));

    // 内容区统一为微软雅黑 11pt 后，固定宽度无法容纳完整按钮文本。
    // 按当前语言实际文本度量所需宽度，两个按钮取较大值并统一，
    // 切换语言时同步重算（左右内边距及圆角留量 +36px）。
    const QFont font(QStringLiteral("Microsoft YaHei"), 11);
    const QFontMetrics fm(font);
    const int textWidth = qMax(fm.horizontalAdvance(m_copyButton->text()),
                               fm.horizontalAdvance(m_okButton->text()));
    m_copyButton->setFixedWidth(textWidth + 36);
    m_okButton->setFixedWidth(textWidth + 36);
}

void CarbonUISystemInfoDialog::refreshInfoText()
{
    m_infoText->setText(CarbonUISystemInfo::renderSystemInfo(m_data, m_labels));
}
