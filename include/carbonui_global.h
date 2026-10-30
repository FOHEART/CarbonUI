/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  carbonui_global.h
 * @brief 导出宏与模块分组定义。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 *
 * @details
 * 本头文件只做预处理层面的事情：把编译期开关 `CARBONUI_DLL_EXPORT`（构建 DLL
 * 时由 CMake 以 PRIVATE 方式定义）与 `CARBONUI_STATIC_LIB`（静态库时以 PUBLIC
 * 方式定义）翻译成统一的 `CARBONUI_DLL_API`，供所有对外符号使用。
 *
 * 依赖：仅依赖 QtCore 的 `qglobal.h`，不引入任何 Qt 控件类型，可在任意 TU 中
 * 安全包含。无运行期状态，可在多线程中并发包含。
 *
 * @note 使用方（宿主程序）不需要定义任何宏：静态库构建会把
 *       `CARBONUI_STATIC_LIB` 作为 INTERFACE 定义传播出去；共享库构建时
 *       头文件自动退化为 `__declspec(dllimport)`。
 */

/**
 * @defgroup carbonui CarbonUI 控件样式库
 * @brief Qt 5 Widgets 的暗色样式助手与可折叠分组框。
 *
 * @details
 * 库内含两个彼此独立的组件：
 * - ::CarbonUIClass —— 全静态样式门面，按控件逐个应用 QSS；
 * - ::QGroupBoxCollapser —— 把已有的 QGroupBox 改造成可折叠面板的控制器对象。
 *
 * 两者不共享状态，也不强制配色主题；调用方按需选用。
 */

#pragma once

#include <QtCore/qglobal.h>



 //////////////////////////////////////////////////
 // generic preprocessor defines

 // use XSENS_32BIT and XSENS_64BIT to check for 32/64 bit builds in your application
 // on non-windows systems these should be defined

/**
 * @brief 对外符号的导出/导入修饰宏。
 *
 * @details
 * 取值由两个编译期开关决定，四种组合对应四种语义：
 * - 定义了 `CARBONUI_DLL_EXPORT`：本工程正在编译库本体，Win32 下展开为
 *   `__declspec(dllexport)`，其它平台展开为 `__attribute__((visibility("default")))`；
 * - 未定义 `CARBONUI_DLL_EXPORT` 且定义了 `CARBONUI_STATIC_LIB`：静态库，
 *   展开为空（无修饰）；
 * - 未定义 `CARBONUI_DLL_EXPORT`、未定义 `CARBONUI_STATIC_LIB` 且在 Win32：
 *   共享库使用方，展开为 `__declspec(dllimport)`；
 * - 其它平台：展开为空。
 *
 * @note 已存在 `CARBONUI_DLL_API` 时不再重定义，便于宿主工程覆盖。
 */
#ifndef CARBONUI_DLL_API
#ifdef CARBONUI_DLL_EXPORT
#ifdef _WIN32
	//#pragma message("CARBONUI_DLL_API export in CARBONUIconfig.h")
#define CARBONUI_DLL_API __declspec(dllexport)
#else
	//#pragma message("CARBONUI_DLL_API linux export in CARBONUIconfig.h")
#define CARBONUI_DLL_API __attribute__((visibility("default")))
#endif
#else	// ifdef CARBONUI_DLL_EXPORT
#ifdef CARBONUI_STATIC_LIB
	//#pragma message("CARBONUI_DLL_API static in CARBONUIconfig.h")
#define CARBONUI_DLL_API
#elif defined(_WIN32)
	//#pragma message("CARBONUI_DLL_API import in CARBONUIconfig.h")
#define CARBONUI_DLL_API __declspec(dllimport)
#else
	//#pragma message("CARBONUI_DLL_API import/static for linux in CARBONUIconfig.h")
#define CARBONUI_DLL_API
#endif
#endif	// ifdef CARBONUI_DLL_EXPORT - else
#endif	// ifndef CARBONUI_DLL_API
