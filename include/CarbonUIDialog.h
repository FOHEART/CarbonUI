/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIDialog.h
 * @brief 深色"关于"式对话框：自带标题栏（标题 + 关闭按钮）与键值行正文。
 * @author CarbonUI
 * @date  2026-11-15
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 复刻 CarbonUI 产品线里的 About 对话框外观：无系统边框（FramelessWindowHint）、
 * 9px 圆角深色面板，顶部 46px 纯黑标题条（左侧白色加粗标题、右侧白色关闭
 * 按钮），下方 #212121 正文区按行排布 "名称: 取值" 文本。颜色取自界面规范：
 * 标题条 #0f0f0f、正文 #212121、标题 #ffffff、行文本 #cdcdcd。
 *
 * 与 CarbonUIClass 静态样式助手的分工：那些函数只往调用方已建好的控件上刷
 * QSS；本类自己负责结构与交互（圆角透明窗、标题拖拽移动、关闭按钮），调用方
 * 只需构造 + 追加行 + show()/exec()。
 *
 * 用法示例：
 * @code
 * auto* dlg = new CarbonUIDialog(QStringLiteral("ABOUT"), parent);
 * dlg->addInfoRow(QStringLiteral("Program version"), QStringLiteral("2025.2"));
 * dlg->addInfoRow(QStringLiteral("License"), QStringLiteral("Perpetual"));
 * dlg->exec();
 * @endcode
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIDIALOG_H
#define CARBONUIDIALOG_H

#include "carbonui_global.h"

#include <QtCore/QPoint>
#include <QtCore/QString>
#include <QtWidgets/QDialog>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

/**
 * @brief 深色无边框对话框：自定义标题栏 + 键值行正文。
 *
 * @details
 * 控件树（全部在本类 ctor 内装配）：
 * @code
 * CarbonUIDialog (FramelessWindowHint, 圆角透明窗)
 *  └─ QWidget "CarbonUIDialogFrame"      #212121 面板, 9px 圆角, 1px 黑边
 *      ├─ QWidget "CarbonUIDialogHeader" #0f0f0f 标题条, 固定高 46px
 *      │   ├─ QLabel "CarbonUIDialogTitle"   白色加粗标题
 *      │   └─ QPushButton "CarbonUIDialogClose" 白色 ✕，点击关闭
 *      └─ QWidget "CarbonUIDialogBody"   正文区
 *          ├─ QLabel "CarbonUIDialogRow" ×N   "名称: 取值"
 *          └─ ...
 * @endcode
 *
 * 交互：按住标题条可拖动窗口（无系统边框时的补偿）；Esc 与关闭按钮都走
 * QDialog 默认的 reject 路径。默认尺寸由内容与 @ref minimumWidth 共同决定
 *（`adjustSize()` 后取 sizeHint）。
 */
class CARBONUI_DLL_API CarbonUIDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @brief 构造对话框并完成内部装配。
     *
     * @param[in] title  标题栏文字，可为空串（标题标签隐藏文字但仍占位）；
     *                   通常传产品化的大写标题，如 "ABOUT"。
     * @param[in] parent 父窗口指针，**可为 NULL**；非 NULL 时对话框居中于父窗口。
     */
    explicit CarbonUIDialog(const QString& title, QWidget* parent = nullptr);

    /**
     * @brief 更新标题栏文字。
     * @param[in] title 新标题，可为空串；立即反映到标题标签。
     */
    void setTitleText(const QString& title);

    /**
     * @brief 查询当前标题栏文字。
     * @return 标题标签的当前文本，可能为空串。
     */
    QString titleText() const;

    /**
     * @brief 追加一行 "名称: 取值" 信息行。
     *
     * @details
     * 行文本整体使用 #cdcdcd，与参考界面的 About 对话框一致；不做自动换行，
     * 超长取值会撑宽对话框（例如 40 位修订号），把对话框放在滚动容器里不是
     * 本类的职责。
     *
     * @param[in] label 行名称，可为空串；为空时输出 ": 取值"。
     * @param[in] value 行取值，可为空串；为空时输出 "名称: "。
     */
    void addInfoRow(const QString& label, const QString& value);

    /**
     * @brief 追加一行原样文本（不再拼接 "名称: 取值"）。
     * @param[in] text 行文本，可为空串（会创建一行空标签，通常应避免）。
     */
    void addInfoRow(const QString& text);

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
    /// @brief 装配控件树与样式（仅构造时调用一次）。
    void buildUi();

    QWidget* m_frame;          ///< 圆角面板容器（objectName: CarbonUIDialogFrame）。
    QWidget* m_header;         ///< 标题条（objectName: CarbonUIDialogHeader），不取得所有权。
    QLabel* m_titleLabel;      ///< 标题标签（objectName: CarbonUIDialogTitle），不取得所有权。
    QPushButton* m_closeButton;///< 关闭按钮（objectName: CarbonUIDialogClose），不取得所有权。
    QWidget* m_body;           ///< 正文容器（objectName: CarbonUIDialogBody），不取得所有权。
    QVBoxLayout* m_bodyLayout; ///< 正文行布局，不取得所有权。
    QPoint m_dragOffset;       ///< 拖拽时鼠标相对窗口左上角的偏移。
    bool m_dragging;           ///< 是否正在拖拽标题条。
};

#endif // CARBONUIDIALOG_H
