/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIAboutQtDialog.h
 * @brief 深色「关于 Qt」弹出对话框：一行代码弹出，调用方无需任何临时风格设置。
 * @author CarbonUI
 * @date  2026-11-17
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 复刻 CarbonUI 参考图（tools/reference/aboutqt.png）的「关于 Qt」对话框：
 * 无系统边框、9px 圆角深色面板，顶部 46px 纯黑标题条（左侧白色加粗大写
 * 标题 "ABOUT QT"、右侧关闭按钮），正文区依次为 Qt 徽标 + "About Qt" 标题行
 * 与富文本说明段落（含可点击的 download.qt.io / qt.io 超链接）。配色与结构
 * 契约同 CarbonUIDialog（标题条 #0f0f0f、正文 #212121、文字 #cdcdcd），
 * 但正文是富文本而非键值行，因此不复用 CarbonUIDialog 而是独立装配。
 *
 * Qt 徽标（绿色圆角方块 + 白色 "Qt" 字样）在构造时用 QPainter 程序化绘制，
 * 不依赖任何图片资源，库的 rcc 资源无需扩充。
 *
 * 对话框文案为固定的英文官方表述（与 Qt 官方 About Qt 对话框一致），不随
 * 宿主程序语言切换；运行时 Qt 版本经 qVersion() 动态填充。
 *
 * 用法示例（第三方应用一行调用，无需先 apply 任何样式）：
 * @code
 * #include "CarbonUIAboutQtDialog.h"
 *
 * // 帮助菜单动作里直接：
 * CarbonUIAboutQtDialog::showAboutQt(this);
 * @endcode
 *
 * 也可自行构造实例控制显示时机：
 * @code
 * auto* dlg = new CarbonUIAboutQtDialog(parent);
 * dlg->show();               // 或 exec() 模态
 * @endcode
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIABOUTQTDIALOG_H
#define CARBONUIABOUTQTDIALOG_H

#include "carbonui_global.h"

#include <QtCore/QPoint>
#include <QtWidgets/QDialog>

class QEvent;
class QLabel;
class QMouseEvent;
class QPushButton;
class QWidget;

/**
 * @brief 深色无边框「关于 Qt」对话框：自带标题栏 + 徽标 + 富文本正文。
 *
 * @details
 * 控件树（全部在本类 ctor 内装配）：
 * @code
 * CarbonUIAboutQtDialog (FramelessWindowHint, 圆角透明窗)
 *  └─ QWidget "CarbonUIDialogFrame"       #212121 面板, 9px 圆角, 1px 黑边
 *      ├─ QWidget "CarbonUIDialogHeader"  #0f0f0f 标题条, 固定高 46px
 *      │   ├─ QLabel "CarbonUIDialogTitle"      白色加粗 "ABOUT QT"
 *      │   └─ QPushButton "CarbonUIDialogClose" 白色 ✕，点击关闭
 *      └─ QWidget "CarbonUIAboutQtBody"   正文区
 *          ├─ QLabel（QPixmap）           程序化绘制的 Qt 徽标
 *          ├─ QLabel                      "About Qt" 粗体标题行
 *          └─ QLabel "CarbonUIAboutQtText" 富文本段落（含超链接）
 * @endcode
 *
 * 交互：按住标题条可拖动窗口；Esc 与关闭按钮都走 QDialog 默认的 reject
 * 路径；超链接经 openExternalLinks 交给系统浏览器。objectName 沿用
 * CarbonUIDialog 的命名约定（标题条/关闭按钮同名），便于宿主测试统一查找。
 */
class CARBONUI_DLL_API CarbonUIAboutQtDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造对话框并完成内部装配（含徽标绘制与富文本填充）。
     * @param[in] parent 父窗口指针，**可为 NULL**；非 NULL 时对话框居中于父窗口。
     */
    explicit CarbonUIAboutQtDialog(QWidget* parent = nullptr);

    /**
     * @brief 一行代码弹出模态「关于 Qt」对话框，返回后自动销毁。
     *
     * @details
     * 等价于构造 + exec()；栈上对象，exec 返回即析构，调用方无需管理
     * 生命周期，也无需预先对应用或窗口做任何风格设置。
     *
     * @param[in] parent 父窗口指针，**可为 NULL**；非 NULL 时对话框居中于它。
     */
    static void showAboutQt(QWidget* parent = nullptr);

protected:
    /**
     * @brief 记录标题条拖拽起点。
     * @param[in] event 鼠标事件，不可为 NULL；只有左键且落在标题条内才生效。
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 拖拽移动窗口。
     * @param[in] event 鼠标事件，不可为 NULL；未处于拖拽状态时忽略。
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief 结束拖拽。
     * @param[in] event 鼠标事件，不可为 NULL。
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    /// @brief 装配控件树、绘制徽标并填充富文本（仅构造时调用一次）。
    void buildUi();

    QWidget* m_frame;          ///< 圆角面板容器（objectName: CarbonUIDialogFrame）。
    QWidget* m_header;         ///< 标题条（objectName: CarbonUIDialogHeader），不取得所有权。
    QLabel* m_titleLabel;      ///< 标题标签（objectName: CarbonUIDialogTitle），不取得所有权。
    QPushButton* m_closeButton;///< 关闭按钮（objectName: CarbonUIDialogClose），不取得所有权。
    QPoint m_dragOffset;       ///< 拖拽时鼠标相对窗口左上角的偏移。
    bool m_dragging;           ///< 是否正在拖拽标题条。
};

#endif // CARBONUIABOUTQTDIALOG_H
