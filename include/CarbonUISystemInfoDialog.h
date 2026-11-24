/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUISystemInfoDialog.h
 * @brief 系统信息对话框：CarbonUIDialog 外壳 + 系统信息正文，内置双语接口。
 * @ingroup carbonui
 *
 * @details
 * 复刻产品线「帮助 → 系统信息」弹窗：CarbonUIDialog 深色圆角外壳内放
 * 系统信息分组框（QTextEdit 只读正文）与「复制到剪贴板 / 确定」按钮行。
 * 数据经 CarbonUISystemInfo::collectSystemInfo() 在构造时同步采集一次并
 * 缓存，语言切换只重新渲染，不重新采集。
 *
 * 双语接口：与 CarbonUIUsageStatusBar 同范式——不依赖任何宿主翻译器，
 * 构造时按 QLocale::system() 初始化，宿主可随时调用 setLanguage()
 * （"zh" 开头视为中文，其余英文）整体重建标题 / 按钮 / 正文文案。
 * 语言切换信号（如宿主的 LanguageManager::languageChanged）由宿主连接。
 *
 * 用法示例：
 * @code
 * auto* dlg = new CarbonUISystemInfoDialog(this);
 * connect(&LanguageManager::getInstance(), &LanguageManager::languageChanged,
 *         dlg, &CarbonUISystemInfoDialog::setLanguage);
 * dlg->show();
 * @endcode
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用；构造时的
 * 系统信息采集含带超时的外部命令调用（2~3 秒）。
 */

#ifndef CARBONUISYSTEMINFODIALOG_H
#define CARBONUISYSTEMINFODIALOG_H

#include "CarbonUIDialog.h"
#include "CarbonUISystemInfo.h"
#include "carbonui_global.h"

#include <QtCore/QString>

class QGroupBox;
class QPushButton;
class QTextEdit;

/**
 * @brief 系统信息对话框：展示本机软硬件信息，支持复制到剪贴板与双语切换。
 */
class CARBONUI_DLL_API CarbonUISystemInfoDialog : public CarbonUIDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造对话框：采集系统信息并按系统语言渲染。
     * @param[in] parent 父窗口，可为 NULL；非 NULL 时对话框居中于父窗口。
     */
    explicit CarbonUISystemInfoDialog(QWidget* parent = nullptr);

  public slots:
    /**
     * @brief 切换显示语言并整体重建文案（标题 / 按钮 / 正文）。
     * @param[in] languageCode 语言代码，"zh" 开头视为中文，其余英文。
     * @note 只重新渲染缓存的采集数据，不重新采集系统信息。
     */
    void setLanguage(const QString& languageCode);

    /// @brief 把正文文本框内容写入系统剪贴板。
    void copyToClipboard();

    /// @brief 关闭对话框。
    void requestClose();

  private:
    /// @brief 装配正文控件树与样式（仅构造时调用一次）。
    void buildBody();

    /// @brief 按当前语言刷新标题 / 分组框 / 按钮文案与按钮宽度。
    void retranslateUi();

    /// @brief 用缓存的采集数据按当前语言重绘正文文本。
    void refreshInfoText();

    CarbonUISystemInfo::SystemInfoData m_data;   ///< 构造时采集并缓存的系统信息。
    CarbonUISystemInfo::Labels m_labels;         ///< 当前语言文案。
    QString m_language;                          ///< 当前语言代码。
    QGroupBox* m_groupBox = nullptr;             ///< 系统信息分组框，归本对话框所有。
    QTextEdit* m_infoText = nullptr;             ///< 正文文本框，归本对话框所有。
    QPushButton* m_copyButton = nullptr;         ///< 复制按钮，归本对话框所有。
    QPushButton* m_okButton = nullptr;           ///< 确定按钮，归本对话框所有。
};

#endif  // CARBONUISYSTEMINFODIALOG_H
