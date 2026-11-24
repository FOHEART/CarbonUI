/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIWindowCorners.h
 * @brief 无边框窗口圆角外观助手：DWM 系统圆角 + 软件裁剪回退 + 圆角描边。
 * @author CarbonUI
 * @date  2026-11-24
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 适用于「Qt::FramelessWindowHint + 不透明底色」的自绘窗口（如无系统标题栏的
 * 主窗口）。圆角有两条实现路径，由 CarbonUIWindowCorners::applyDwmRoundedCorners
 * 的返回值决定：
 * - Windows 11 及以上：设置 DWM 系统圆角（DWMWCP_ROUND），由窗口合成器做
 *   抗锯齿裁剪，边缘完全平滑；同时移除 DWM 默认在窗口外沿绘制的 1px 边框线
 *   （亮色壁纸下会看到一圈背景色缝隙）。此时宿主窗口不应再调用 setMask。
 * - 旧系统：返回 false，宿主回退到 CarbonUIWindowCorners::updateWindowMask
 *   （1-bit QRegion 软件裁剪，圆角处有轻微锯齿）+ CarbonUIWindowCorners::
 *   paintWindowFrame 自绘圆角描边。
 *
 * 与 CarbonUIDialog（WA_TranslucentBackground + QSS border-radius）的关系：
 * 对话框类内容简单的窗口用半透明路径即可获得平滑圆角；但持续动画的 OpenGL
 * 视图会让分层窗口每帧走 UpdateLayeredWindow 导致闪屏，这类宿主窗口必须保持
 * 不透明，圆角交给本助手。两条路径互斥，按窗口内容选型。
 *
 * 线程约定：所有接口都会触碰 QWidget / 系统 API，只允许在 GUI 主线程调用。
 * 错误处理：window / painter 传 NULL 时静默返回，调用方无需判空。
 */

#ifndef CARBONUIWINDOWCORNERS_H
#define CARBONUIWINDOWCORNERS_H

#include "carbonui_global.h"

#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QWidget>

/**
 * @brief 无边框窗口圆角外观助手（全静态、无数据成员）。
 *
 * @details
 * 典型用法（宿主窗口持有返回值缓存 dwmActive 标志）：
 * @code
 * // 构造：Qt::FramelessWindowHint 之后立即调用一次
 * m_dwmActive = CarbonUIWindowCorners::applyDwmRoundedCorners(this);
 *
 * // resizeEvent / WindowStateChange：刷新软件裁剪（DWM 生效时内部自动跳过）
 * CarbonUIWindowCorners::updateWindowMask(this, radius, m_dwmActive);
 *
 * // paintEvent：绘制圆角底色与 1px 边框
 * CarbonUIWindowCorners::paintWindowFrame(this, &painter, background,
 *                                         borderColor, radius, m_dwmActive);
 * @endcode
 *
 * 线程安全：非线程安全，只能在 GUI 主线程使用。
 */
class CARBONUI_DLL_API CarbonUIWindowCorners {
  public:
	/// 无边框窗口默认圆角半径（像素），与 CarbonUIDialog 面板圆角一致。
	static constexpr int kDefaultCornerRadius = 9;

	/// Windows 11 DWM 系统圆角（DWMWCP_ROUND）在 96DPI 下的标称半径。
	/// DWM 生效时宿主描边应使用该半径，避免自绘圆角弧线被系统裁剪出豁口。
	static constexpr qreal kDwmCornerRadius = 8.0;

	/**
	 * @brief 尝试对顶层窗口启用 Windows 11 DWM 系统圆角。
	 *
	 * @details 设置 DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND（合成器
	 *          抗锯齿裁剪），成功后追加设置 DWMWA_BORDER_COLOR =
	 *          DWMWA_COLOR_NONE 移除 DWM 默认的 1px 外沿边框线。生效后窗口
	 *          由 DWM 裁剪，宿主不应再 setMask（1-bit mask 反而引入锯齿）。
	 *
	 * @param[in,out] window 顶层 QWidget，**可为 NULL**（静默返回 false）；
	 *                        内部会强制创建原生窗口句柄。
	 *
	 * @return true 表示 DWM 圆角已生效（Win11+）；false 表示系统不支持
	 *         （Win10 及更早 / 非 Windows 平台），调用方应走软件裁剪回退。
	 */
	static bool applyDwmRoundedCorners(QWidget* window);

	/**
	 * @brief 按窗口状态刷新圆角软件裁剪区域（QRegion mask 回退路径）。
	 *
	 * @details 最大化 / 全屏 / 尺寸无效 / @p dwmRoundedActive 为 true（DWM
	 *          已在裁剪，mask 反而引入锯齿）时清除 mask；否则按 @p radius
	 *          生成圆角 mask。应在 resizeEvent 与窗口状态变化后调用。
	 *
	 * @param[in,out] window            顶层 QWidget，**可为 NULL**（静默返回）。
	 * @param[in]     radius            圆角半径（像素）。
	 * @param[in]     dwmRoundedActive  @ref applyDwmRoundedCorners 的返回值。
	 */
	static void updateWindowMask(QWidget* window, int radius, bool dwmRoundedActive);

	/**
	 * @brief 绘制无边框窗口的圆角底色与 1px 边框。
	 *
	 * @details 最大化时画直角矩形；否则画圆角矩形——DWM 圆角生效时半径取
	 *          kDwmCornerRadius（与系统裁剪对齐，防止自绘弧线被裁出豁口），
	 *          软件裁剪时取 @p radius。painter 需由调用方创建，本函数内部
	 *          开启抗锯齿。等价于宿主 paintEvent 中的整块窗口描边逻辑。
	 *
	 * @param[in]     window            顶层 QWidget，**可为 NULL**（静默返回）。
	 * @param[in,out] painter           已构造在 window 上的 QPainter，**可为 NULL**。
	 * @param[in]     backgroundColor   窗口底色（如 #212121）。
	 * @param[in]     borderColor       1px 边框颜色（如 #000000）。
	 * @param[in]     radius            圆角半径（像素）。
	 * @param[in]     dwmRoundedActive  @ref applyDwmRoundedCorners 的返回值。
	 */
	static void paintWindowFrame(QWidget* window, QPainter* painter,
								 const QColor& backgroundColor,
								 const QColor& borderColor,
								 int radius, bool dwmRoundedActive);
};

#endif // CARBONUIWINDOWCORNERS_H
