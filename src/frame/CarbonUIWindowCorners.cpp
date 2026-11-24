/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIWindowCorners.cpp
 * @brief CarbonUIWindowCorners 实现：DWM 系统圆角 + 软件裁剪回退 + 圆角描边。
 * @author CarbonUI
 * @date  2026-11-24
 * @ingroup carbonui
 *
 * @details
 * 实现说明见 CarbonUIWindowCorners.h；Windows 专属代码集中在
 * applyDwmRoundedCorners（dwmapi），mask / 描边两条路径全平台可用。
 */

#include "CarbonUIWindowCorners.h"

#ifdef _WIN32
#include <qt_windows.h>
#include <dwmapi.h>
#endif

bool CarbonUIWindowCorners::applyDwmRoundedCorners(QWidget* window)
{
#ifdef _WIN32
	if (window == nullptr)
	{
		return false;
	}
	const HWND hwnd = reinterpret_cast<HWND>(window->winId());
	if (hwnd == nullptr)
	{
		return false;
	}
	// DWM 系统圆角：旧系统（Win10 及更早）返回 E_INVALIDARG，函数整体返回 false。
	const DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
	if (::DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
								&preference, sizeof(preference)) != S_OK)
	{
		return false;
	}
	// DWM 圆角默认会在窗口外沿画一条 1px 边框（实测右/上边缘各出现一条亮色
	// hairline，亮色壁纸下像一圈背景色缝隙），设为无边框让内容铺到边缘。
	const COLORREF borderColor = DWMWA_COLOR_NONE;
	::DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
	return true;
#else
	Q_UNUSED(window);
	return false;
#endif
}

void CarbonUIWindowCorners::updateWindowMask(QWidget* window, int radius,
											 bool dwmRoundedActive)
{
	if (window == nullptr)
	{
		return;
	}
	if (dwmRoundedActive || window->isMaximized()
		|| window->windowState().testFlag(Qt::WindowFullScreen)
		|| window->width() <= 0 || window->height() <= 0)
	{
		window->clearMask();
		return;
	}
	QPainterPath path;
	path.addRoundedRect(QRectF(window->rect()), static_cast<qreal>(radius),
						static_cast<qreal>(radius));
	window->setMask(QRegion(path.toFillPolygon().toPolygon()));
}

void CarbonUIWindowCorners::paintWindowFrame(QWidget* window, QPainter* painter,
											 const QColor& backgroundColor,
											 const QColor& borderColor,
											 int radius, bool dwmRoundedActive)
{
	if (window == nullptr || painter == nullptr)
	{
		return;
	}
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(QPen(borderColor, 1));
	painter->setBrush(backgroundColor);

	const QRect frame = window->rect().adjusted(0, 0, -1, -1);
	if (window->isMaximized())
	{
		painter->drawRect(frame);
	}
	else
	{
		const qreal effectiveRadius = dwmRoundedActive
										  ? kDwmCornerRadius
										  : static_cast<qreal>(radius);
		painter->drawRoundedRect(frame, effectiveRadius, effectiveRadius);
	}
}
