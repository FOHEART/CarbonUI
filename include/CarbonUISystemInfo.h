/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUISystemInfo.h
 * @brief 系统软硬件信息采集与渲染：纯逻辑，无 UI 依赖，可单测。
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIProcessStats 同一实现范式：平台采集（Windows 注册表 /
 * SetupAPI / kernel32，Linux /proc 与命令行，macOS 命令行）只负责把
 * 客观数据装进 SystemInfoData，文案与格式完全由 Labels + renderSystemInfo()
 * 完成，因此渲染逻辑可以在无窗口环境下单测。
 *
 * 双语支持：CarbonUI 不携带 .ts/.qm 翻译文件，由 labelsForLanguage()
 * 内置中英文映射；调用方（如 CarbonUISystemInfoDialog）只需在自己的
 * 语言切换槽里调用一次即可整体重建文案。
 *
 * 线程约定：collectSystemInfo() 内含带超时的外部命令调用（QProcess），
 * 只允许在 GUI 主线程调用。
 */

#ifndef CARBONUISYSTEMINFO_H
#define CARBONUISYSTEMINFO_H

#include "carbonui_global.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QStringList>

/**
 * @brief 系统软硬件信息采集与文案渲染。
 */
namespace CarbonUISystemInfo
{

/// @brief 一次采集得到的本机软硬件信息；各字段独立标记有效性。
/// @details 所有字段均为客观数据，不含任何文案；采集失败的字段保持
///          无效值（空串 / 0 / false），由渲染层按语言显示占位提示。
struct SystemInfoData
{
  QString osType;             ///< 操作系统类型，如 "windows"（QSysInfo::productType）。
  QString kernelVersion;      ///< 内核版本（QSysInfo::kernelVersion）。
  QString cpuArchitecture;    ///< CPU 架构，如 "x86_64"（QSysInfo::currentCpuArchitecture）。

  QString cpuModel;           ///< CPU 型号；获取失败为空串。
  int logicalCores = 0;       ///< 逻辑核心数；获取失败为 0。
  int physicalCores = 0;      ///< 物理核心数；获取失败为 0（渲染时不输出该行）。

  double totalMemoryGB = 0.0; ///< 实际安装物理内存，单位 GB；获取失败为 0。
  bool memoryValid = false;   ///< 内存容量是否有效。

  QStringList gpuNames;       ///< 显卡友好名称列表；获取失败为空列表。

  QString storageSummary;     ///< 存储概要：Windows 为盘符串（如 "C D E "），Unix 为根分区 df 行。
  bool storageValid = false;  ///< 存储概要是否有效。
};

/// @brief 渲染文案标签，由调用方按当前语言经 labelsForLanguage() 取词。
struct Labels
{
  // 分节标题（渲染时自动加 "=== xxx ===" 包裹）。
  QString osSection;
  QString cpuSection;
  QString memorySection;
  QString gpuSection;
  QString storageSection;

  // 字段标签（渲染时以 "标签: 值" 形式输出）。
  QString osName;
  QString kernelVersion;
  QString architecture;
  QString cpuModel;
  QString logicalCores;
  QString physicalCores;
  QString totalMemory;
  QString gpuName;
  QString detectedDrives;

  // 各分节整体不可用时的占位提示。
  QString cpuUnavailable;
  QString memoryUnavailable;
  QString gpuUnavailable;
  QString storageUnavailable;
};

/**
 * @brief 判断语言代码是否为中文（"zh" 开头，含 "zh_CN" / "zh_TW" 等）。
 */
bool CARBONUI_DLL_API isChineseLanguageCode(const QString& languageCode);

/**
 * @brief 按语言代码取内置文案标签。
 * @param[in] languageCode 语言代码，"zh" 开头视为中文，其余一律英文。
 * @return 对应语言的 Labels。
 */
Labels CARBONUI_DLL_API labelsForLanguage(const QString& languageCode);

/**
 * @brief 采集本机软硬件信息（同步、阻塞）。
 * @details 外部命令调用（wmic / free / lspci / df / system_profiler）均带
 *          2~3 秒超时，最坏情况约数秒；只允许在 GUI 主线程调用。
 *          各子系统独立采集，互不影响：CPU 失败不影响内存与显卡。
 * @return 填充好的 SystemInfoData。
 */
SystemInfoData CARBONUI_DLL_API collectSystemInfo();

/**
 * @brief 把采集结果渲染成对话框展示的整段文本。
 * @param[in] data 采集结果；无效字段按语言显示占位提示。
 * @param[in] labels 当前语言文案。
 * @return 五个分节（操作系统 / CPU / 内存 / 显卡 / 存储）的完整文本，
 *         分节间以空行分隔，行尾均带换行。
 */
QString CARBONUI_DLL_API renderSystemInfo(const SystemInfoData& data,
                                          const Labels& labels);

}  // namespace CarbonUISystemInfo

#endif  // CARBONUISYSTEMINFO_H
