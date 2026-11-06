/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIVersion.h
 * @brief 工程版本号唯一来源（Single Source of Truth）。
 * @author CarbonUI
 * @date  2026-11-06
 * @version 见本文件宏定义（本文件即版本来源，不再单独维护 @version 数字）
 *
 * @details
 * 全工程所有与版本号相关的部分都以本文件为准，升版时只改这里的三个数字与
 * `CARBONUI_VERSION` 字符串行（共 4 行），其余位置自动跟随：
 * - CMake：`project(VERSION)`、目标 `VERSION`/`SOVERSION`、Windows DLL 版本
 *   资源（cmake/version.rc.in，写入 dll 文件属性）均在配置期从本文件解析；
 * - Doxygen：`PROJECT_NUMBER` 由 scripts/gen_docs.bat 解析本文件后经环境变量
 *   `CARBONUI_VERSION` 注入 Doxyfile 的 `$(CARBONUI_VERSION)`；
 * - 源码分发包：tools/package_source.ps1 命名 `CarbonUI_v<版本>.zip`；
 * - 远程 tag / GitHub Release：scripts/release.bat 以 `v<版本>` 打 tag 并推送，
 *   .github/workflows/release.yml 校验 tag 与本文件一致后发布同名资产。
 *
 * 依赖：无（纯预处理宏，任何 TU 可安全包含，无运行期状态）。
 */

#pragma once

/**
 * @defgroup carbonui_version 版本号宏
 * @brief 版本号唯一来源，升版只改前三个数字宏与 CARBONUI_VERSION 字符串。
 * @{
 */

/// 主版本号（语义化版本第 1 位）。
#define CARBONUI_VERSION_MAJOR 0
/// 次版本号（语义化版本第 2 位）。
#define CARBONUI_VERSION_MINOR 2
/// 修订号（语义化版本第 3 位）。
#define CARBONUI_VERSION_PATCH 1

/**
 * 版本号字符串形式（如 "0.2.1"）。
 *
 * @note 这是脚本（package/release/gen_docs）与资源文件解析的锚点行，
 *       必须保持 `#define CARBONUI_VERSION "x.y.z"` 的固定格式。
 */
#define CARBONUI_VERSION "0.2.1"

/// git tag / GitHub Release 形式（如 "v0.2.1"）。
#define CARBONUI_VERSION_TAG "v" CARBONUI_VERSION

/* 编译期由三个数字宏拼出的版本字符串，用于人工核对 CARBONUI_VERSION 是否同步。 */

/**
 * @brief 字符串化辅助宏（内部实现，勿直接使用）。
 * @param x 要字符串化的记号。
 */
#define CARBONUI_STR2_(x) #x

/**
 * @brief 字符串化辅助宏（内部实现，勿直接使用）。
 * @param x 要字符串化的记号（先展开两层宏再字符串化）。
 */
#define CARBONUI_STR2(x)  CARBONUI_STR2_(x)

/**
 * @brief 由三个数字宏编译期拼出的版本字符串（如 "0.2.1"）。
 * @note 供人工核对 CARBONUI_VERSION 与数字宏是否同步：二者应始终相等。
 */
#define CARBONUI_VERSION_STRING \
    CARBONUI_STR2(CARBONUI_VERSION_MAJOR) "." \
    CARBONUI_STR2(CARBONUI_VERSION_MINOR) "." \
    CARBONUI_STR2(CARBONUI_VERSION_PATCH)

/** @} */
