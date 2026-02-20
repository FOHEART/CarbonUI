/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QtCore/qglobal.h>



 //////////////////////////////////////////////////
 // generic preprocessor defines

 // use XSENS_32BIT and XSENS_64BIT to check for 32/64 bit builds in your application
 // on non-windows systems these should be defined

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