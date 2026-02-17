/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(CARBONUI_LIB)
#  define CARBONUI_EXPORT Q_DECL_EXPORT
# else
#  define CARBONUI_EXPORT Q_DECL_IMPORT
# endif
#else
# define CARBONUI_EXPORT
#endif
