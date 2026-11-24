/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIUsageStatusBar.cpp
 * @brief CarbonUIUsageStatusBar 的实现：子控件装配、双定时器驱动、语言映射。
 * @author CarbonUI
 * @date  2026-11-24
 * @ingroup carbonui
 *
 * @details
 * 实现 CarbonUIUsageStatusBar.h 声明的复合控件。构造时建立两个 QTimer：
 * 采样定时器（kDefaultSampleIntervalMs）每帧经 CarbonUIProcessStats::Monitor
 * 采集一次进程与整机读数；轮播定时器（kDefaultCycleIntervalMs）把读数标签在
 * CPU / 进程内存 / 系统可用内存三个项目间轮换。文案按 m_language 从内置
 * zh / en 映射取词，宿主通过 setLanguage() 接入自己的语言切换信号。
 */

#include "CarbonUIUsageStatusBar.h"

#include <QtCore/QLocale>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

namespace {

/// @brief 读数与消息标签的公共外观：无边框、12px、透明底，与状态栏底色融合。
const char* const kLabelStyleSheet =
	"border:none; font-size: 12px; background-color: rgba(0, 0, 0, 0);";

/// @brief 判断语言代码是否为中文（"zh" 开头，兼容 "zh_CN" 等）。
bool isChineseLanguageCode(const QString& langCode)
{
	return langCode.startsWith(QLatin1String("zh"), Qt::CaseInsensitive);
}

}  // namespace

CarbonUIUsageStatusBar::CarbonUIUsageStatusBar(QWidget* parent)
	: QWidget(parent)
{
	// 初始语言：宿主未接入 setLanguage 前按系统区域给一个合理默认。
	m_language = isChineseLanguageCode(QLocale::system().name())
					 ? QStringLiteral("zh")
					 : QStringLiteral("en");

	QHBoxLayout* outerLayout = new QHBoxLayout(this);
	outerLayout->setContentsMargins(0, 0, 0, 0);
	outerLayout->setSpacing(0);

	QWidget* content = new QWidget(this);
	QHBoxLayout* contentLayout = new QHBoxLayout(content);
	contentLayout->setContentsMargins(0, 0, 0, 0);

	m_messageLabel = new QLabel(defaultMessage(), content);
	m_messageLabel->setStyleSheet(QLatin1String(kLabelStyleSheet));

	m_usageLabel = new QLabel(content);
	// 最小宽度按最长的英文读数（"AVAIL 8123.4 MB"）预留，避免轮播切换时抖动。
	m_usageLabel->setMinimumWidth(112);
	m_usageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_usageLabel->setStyleSheet(QLatin1String(kLabelStyleSheet));

	contentLayout->addWidget(m_messageLabel);
	contentLayout->addStretch(1);
	contentLayout->addWidget(m_usageLabel);
	outerLayout->addWidget(content);

	// 采样定时器：独立于轮播节奏，保证切到任一项目时数值都是最新的。
	m_sampleTimer = new QTimer(this);
	connect(m_sampleTimer, &QTimer::timeout, this, &CarbonUIUsageStatusBar::sampleUsage);
	m_sampleTimer->start(m_sampleIntervalMs);

	// 轮播定时器：CPU → 进程内存 → 系统可用内存循环，每项停留 m_cycleIntervalMs。
	m_cycleTimer = new QTimer(this);
	connect(m_cycleTimer, &QTimer::timeout, this, &CarbonUIUsageStatusBar::switchUsageDisplay);
	m_cycleTimer->start(m_cycleIntervalMs);

	// 首帧立刻给出内存读数；CPU 百分比需要下一帧才有基线。
	sampleUsage();
}

CarbonUIUsageStatusBar::~CarbonUIUsageStatusBar()
{
}

void CarbonUIUsageStatusBar::setMessage(const QString& message)
{
	m_customMessage = message;
	// 空字符串等效恢复默认消息，保持"清空即回到默认"的直觉语义。
	m_messageLabel->setText(m_customMessage.isEmpty() ? defaultMessage()
													  : m_customMessage);
}

QString CarbonUIUsageStatusBar::language() const
{
	return m_language;
}

QString CarbonUIUsageStatusBar::defaultMessage() const
{
	return isChineseLanguageCode(m_language) ? QStringLiteral("就绪")
											 : QStringLiteral("Ready");
}

void CarbonUIUsageStatusBar::setLanguage(const QString& langCode)
{
	m_language = isChineseLanguageCode(langCode) ? QStringLiteral("zh")
												 : QStringLiteral("en");

	// 默认消息随语言重建；宿主自定义消息保持原样不被覆盖。
	m_messageLabel->setText(m_customMessage.isEmpty() ? defaultMessage()
													  : m_customMessage);

	// 读数标签与单位立即按新语言重绘。
	renderUsageLabel();
}

void CarbonUIUsageStatusBar::sampleUsage()
{
	const int coreCount = qMax(1, QThread::idealThreadCount());
	m_lastUsage = m_usageMonitor.poll(coreCount);
	renderUsageLabel();
}

void CarbonUIUsageStatusBar::switchUsageDisplay()
{
	m_usageDisplayIndex = (m_usageDisplayIndex + 1) % kUsageDisplayCount;
	renderUsageLabel();
}

void CarbonUIUsageStatusBar::renderUsageLabel()
{
	const CarbonUIProcessStats::Labels labels = currentLabels();
	switch (m_usageDisplayIndex)
	{
		case 0:
			m_usageLabel->setText(CarbonUIProcessStats::cpuText(m_lastUsage, labels));
			m_usageLabel->setToolTip(isChineseLanguageCode(m_language)
										 ? QStringLiteral("当前进程 CPU 占用率")
										 : QStringLiteral("Current process CPU usage"));
			break;
		case 1:
			m_usageLabel->setText(CarbonUIProcessStats::memoryText(m_lastUsage, labels));
			m_usageLabel->setToolTip(isChineseLanguageCode(m_language)
										 ? QStringLiteral("当前进程常驻内存")
										 : QStringLiteral("Current process resident memory"));
			break;
		default:
			m_usageLabel->setText(
				CarbonUIProcessStats::availableMemoryText(m_lastUsage, labels));
			m_usageLabel->setToolTip(isChineseLanguageCode(m_language)
										 ? QStringLiteral("系统可用内存")
										 : QStringLiteral("System available memory"));
			break;
	}
}

CarbonUIProcessStats::Labels CarbonUIUsageStatusBar::currentLabels() const
{
	CarbonUIProcessStats::Labels labels;
	if (isChineseLanguageCode(m_language))
	{
		labels.cpu = QStringLiteral("CPU占用：");
		labels.memory = QStringLiteral("内存占用：");
		labels.available = QStringLiteral("可用物理内存：");
	}
	else
	{
		labels.cpu = QStringLiteral("CPU Usage: ");
		labels.memory = QStringLiteral("Memory Usage: ");
		labels.available = QStringLiteral("Available Physical Memory: ");
	}
	labels.unavailable = QStringLiteral("--");
	return labels;
}
