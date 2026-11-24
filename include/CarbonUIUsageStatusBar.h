/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIUsageStatusBar.h
 * @brief 状态栏附加控件：状态消息 + CPU / 进程内存 / 系统可用内存三项轮播读数。
 * @author CarbonUI
 * @date  2026-11-24
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 模块边界：QWidget 复合控件，配合 QMainWindow::statusBar()->addPermanentWidget()
 * 使用；进程指标与整机可用内存的采样、换算由 @ref CarbonUIProcessStats 完成，
 * 本控件只负责定时采样驱动、轮播切换与按语言格式化显示。
 *
 * 显示形态：左侧为状态消息标签，右侧为读数标签；读数不平铺——CPU 占用 /
 * 进程常驻内存 / 系统可用内存三个项目轮流展示，每项停留 cycleIntervalMs
 * （默认 2000ms）后切换到下一项循环往复；采样定时器独立（默认 1000ms），
 * 切到哪个项目，哪个项目就用最近一帧采样值。
 *
 * 中英文切换：CarbonUI 不依赖任何翻译器，控件内置 zh / en 文案映射，
 * 宿主把语言切换信号接到 @ref setLanguage(const QString&) 即可（如项目的
 * LanguageManager::languageChanged）；"zh" 开头视为中文，其余一律按英文。
 * 未显式调用时按 QLocale::system() 初始化。
 *
 * 生命周期：由宿主窗口构造并持有（parent 归属），内部 QTimer 与子控件
 * 父对象均为本控件，析构时自动停止并释放。
 *
 * 线程安全：仅 GUI 线程使用，所有槽函数与事件处理均在该线程执行。
 */

#ifndef CARBONUIUSAGESTATUSBAR_H
#define CARBONUIUSAGESTATUSBAR_H

#include "carbonui_global.h"

#include <QtWidgets/QWidget>

#include "CarbonUIProcessStats.h"

class QLabel;
class QTimer;

/**
 * @brief 状态栏附加控件：状态消息 + 资源读数轮播（CPU / 进程内存 / 系统可用内存）。
 *
 * 典型用法（宿主窗口，配合 CarbonUIQStatusBar::applyStyle 统一底色）：
 * @code
 * auto* usageBar = new CarbonUIUsageStatusBar(window);
 * window->statusBar()->addPermanentWidget(usageBar, 300);
 * connect(&LanguageManager::getInstance(), &LanguageManager::languageChanged,
 *         usageBar, &CarbonUIUsageStatusBar::setLanguage);
 * usageBar->setMessage(QStringLiteral("Ready"));
 * @endcode
 *
 * 生命周期：随父窗口构造；子控件与定时器以本控件为父对象，析构时一并释放，
 * 无需手动 delete。
 *
 * 线程安全：非线程安全，只能在 GUI 主线程使用。
 */
class CARBONUI_DLL_API CarbonUIUsageStatusBar : public QWidget {
	Q_OBJECT

  public:
	/// 读数采样定时器默认间隔（毫秒）；CPU 百分比是相邻两帧的区间值，
	/// 间隔越短响应越快、抖动越大，1s 与任务管理器的刷新节奏一致。
	static const int kDefaultSampleIntervalMs = 1000;

	/// 轮播切换默认间隔（毫秒）；每个读数项目停留该时长后切到下一项。
	static const int kDefaultCycleIntervalMs = 2000;

	/// 轮播项目总数：0 = CPU 占用，1 = 进程常驻内存，2 = 系统可用内存。
	static const int kUsageDisplayCount = 3;

	/**
	 * @brief 构造状态栏附加控件并完成子控件、定时器与初始语言装配。
	 *
	 * @param[in] parent 父控件，**可为 NULL**（为 NULL 时作为顶层控件存在，
	 *                   但常规用法是交给 QMainWindow::statusBar() 持有）。
	 */
	explicit CarbonUIUsageStatusBar(QWidget* parent = nullptr);

	/** @brief 析构：子控件与定时器父对象均为本控件，随本控件自动释放。 */
	~CarbonUIUsageStatusBar();

	/**
	 * @brief 设置左侧状态消息文本（空字符串等效清空）。
	 * @param[in] message 新的状态消息；设置后语言切换不再覆盖该自定义文本，
	 *                    传 @ref defaultMessage() 可恢复随语言变化的默认消息。
	 */
	void setMessage(const QString& message);

	/**
	 * @brief 当前显示语言代码（"zh" / "en"）。
	 * @return 构造时按系统区域初始化，之后随 @ref setLanguage 更新。
	 */
	QString language() const;

	/**
	 * @brief 当前语言下的默认状态消息（中文 "就绪" / 英文 "Ready"）。
	 * @return 只读快照；宿主可用它把自定义消息恢复为默认值。
	 */
	QString defaultMessage() const;

  public slots:
	/**
	 * @brief 语言切换接口：按 @p langCode 重建全部可见文案并立即重绘读数。
	 *
	 * @param[in] langCode 语言代码；"zh" 开头视为中文，其余（含 "en" 与
	 *                     未知代码）一律按英文。建议直接连接宿主项目的
	 *                     LanguageManager::languageChanged 这类信号。
	 * @note 读数标签的 tooltip 随语言一并更新；自定义消息文本保持原样。
	 */
	void setLanguage(const QString& langCode);

  private slots:
	/// @brief 采样定时器触发：采集一帧并重绘当前展示的读数项目。
	/// @note 声明为槽以便测试用 QMetaObject::invokeMethod 同步驱动。
	void sampleUsage();

	/// @brief 轮播定时器触发：切到下一个读数项目并立即重绘。
	/// @note 声明为槽以便测试用 QMetaObject::invokeMethod 同步驱动。
	void switchUsageDisplay();

  private:
	/// @brief 按 m_usageDisplayIndex 与当前语言把最近一帧采样重绘到读数标签。
	void renderUsageLabel();

	/// @brief 组装当前语言的 CarbonUIProcessStats::Labels（标签与占位符）。
	CarbonUIProcessStats::Labels currentLabels() const;

	QLabel* m_messageLabel = nullptr;  ///< 左侧状态消息标签。
	QLabel* m_usageLabel = nullptr;    ///< 右侧读数标签（轮播唯一展示位）。

	QTimer* m_sampleTimer = nullptr;  ///< 采样定时器，父对象为本控件。
	QTimer* m_cycleTimer = nullptr;   ///< 轮播切换定时器，父对象为本控件。

	int m_sampleIntervalMs = kDefaultSampleIntervalMs;  ///< 采样间隔（毫秒）。
	int m_cycleIntervalMs = kDefaultCycleIntervalMs;    ///< 轮播切换间隔（毫秒）。

	QString m_language;            ///< 当前语言代码（"zh" / "en"）。
	QString m_customMessage;       ///< 宿主设置的自定义消息；空表示用默认消息。
	int m_usageDisplayIndex = 0;   ///< 当前展示的读数项目索引（0/1/2）。

	CarbonUIProcessStats::Monitor m_usageMonitor;  ///< 进程指标采样状态机。
	CarbonUIProcessStats::Usage m_lastUsage;       ///< 最近一帧采样结果，供切换 / 语言切换重绘。
};

#endif // CARBONUIUSAGESTATUSBAR_H
