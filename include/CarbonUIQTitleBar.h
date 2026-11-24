/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQTitleBar.h
 * @brief 无边框窗口的自定义标题条控件：logo + 标题 + 最小化 / 最大化 / 关闭按钮。
 * @author CarbonUI
 * @date  2026-11-24
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 的标题条同一外观契约：26px 纯黑（\#0f0f0f）横条，最左侧
 * 18px 应用 logo（可省略），其后白色加粗标题，右侧按「最小化 → 最大化 → 关闭」
 * 排列 24px 窗口按钮。窗口圆角通过 `setCornerRadius()` 告知，标题条上两角随之
 * 圆角化；所在窗口最大化时圆角自动归零、拖拽停用，还原后自动恢复。
 *
 * 交互（与 CarbonUIDialog 标题条一致，目标都是所在顶层窗口）：
 * - 左键按住标题条（按钮以外区域）拖拽移动窗口，最大化期间不响应；
 * - 双击标题条（按钮以外区域）在最大化 / 还原之间切换；
 * - 三个窗口按钮分别调用所在窗口的 `showMinimized()`、最大化 / 还原切换与
 *   `close()`。
 *
 * 标题文字自动跟随所在窗口的 `windowTitleChanged`，窗口状态变化（最大化 /
 * 还原）通过事件过滤器自动同步字形与圆角，调用方无需手动刷新；也可显式调用
 * `syncToWindowState()` 强制刷新。通常与 `CarbonUIQMenuBar::adoptExistingMenus()`
 * 配合，把「标题条 + 菜单栏」一起放进 `QMainWindow::setMenuWidget()` 的宿主里。
 *
 * 用法示例：
 * @code
 * auto* menuHost = new QWidget(mainWindow);
 * auto* layout   = new QVBoxLayout(menuHost);
 * layout->setContentsMargins(0, 0, 0, 0);
 * layout->setSpacing(0);
 * auto* titleBar = new CarbonUIQTitleBar(menuHost);
 * titleBar->setCornerRadius(9);
 * titleBar->setLogoPixmap(QPixmap(":/logo.png"));   // 可省略
 * layout->addWidget(titleBar);
 * layout->addWidget(CarbonUIQMenuBar::adoptExistingMenus(mainWindow));
 * mainWindow->setMenuWidget(menuHost);
 * @endcode
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQTITLEBAR_H
#define CARBONUIQTITLEBAR_H

#include "carbonui_global.h"

#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QEvent;
class QLabel;
class QMouseEvent;
class QShowEvent;
class QPushButton;

/**
 * @brief CarbonUI 自定义标题条：无边框窗口的 logo + 标题 + 窗口按钮横条。
 */
class CARBONUI_DLL_API CarbonUIQTitleBar : public QWidget {
    Q_OBJECT
  public:
    /**
     * @brief 构造标题条并完成外观装配。
     * @param[in] parent 父控件指针，**可为 NULL**；所属顶层窗口在进入父链后
     *                   自动绑定（标题文字 / 状态自动同步）。
     */
    explicit CarbonUIQTitleBar(QWidget* parent = nullptr);

    /**
     * @brief 更新标题文字。
     * @param[in] text 新标题，可为空串；标题仍占位。
     */
    void setTitleText(const QString& text);

    /**
     * @brief 查询当前标题文字。
     * @return 标题标签的当前文本，可能为空串。
     */
    QString titleText() const;

    /**
     * @brief 设置标题条最左侧的应用 logo。
     * @param[in] logo logo 位图；空 QPixmap 表示隐藏 logo 位图（保留占位）。
     * @note logo 会等比缩放到 18px；透明底渲染，不遮标题条底色。
     */
    void setLogoPixmap(const QPixmap& logo);

    /**
     * @brief 告知所在窗口的圆角半径。
     * @param[in] radius 圆角半径（像素），0 表示直角；所在窗口最大化时自动归零。
     */
    void setCornerRadius(int radius);

    /**
     * @brief 按所在窗口的当前状态刷新最大化字形、圆角样式与拖拽可用性。
     * @note 状态变化由事件过滤器自动触发；本接口供调用方在需要时手动强制刷新。
     */
    void syncToWindowState();

  protected:
    /**
     * @brief 记录标题条拖拽起点。
     * @param[in] event 鼠标事件，不可为 NULL；只有左键、非最大化且按下点不在
     *                  窗口按钮上才生效。
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 拖拽移动所在窗口。
     * @param[in] event 鼠标事件，不可为 NULL；未处于拖拽状态时忽略。
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief 结束拖拽。
     * @param[in] event 鼠标事件，不可为 NULL。
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /**
     * @brief 双击标题条（窗口按钮以外区域）在最大化 / 还原之间切换。
     * @param[in] event 鼠标事件，不可为 NULL；只有左键且不在窗口按钮上才生效。
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /**
     * @brief 进入顶层窗口链后补绑定所在窗口（构造时窗口未定时兜底）。
     * @param[in] event 事件对象，可为 NULL。
     */
    void showEvent(QShowEvent* event) override;

    /**
     * @brief 监听所在窗口的状态变化，自动刷新最大化字形与圆角。
     * @param[in] watched 事件来源；只处理绑定窗口的 WindowStateChange。
     * @param[in] event 事件对象，不可为 NULL。
     * @return 始终返回 false（事件继续正常分发）。
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    /// @brief 装配控件树与样式（仅构造时调用一次）。
    void buildUi();

    /// @brief 绑定所在顶层窗口：同步标题文字并安装状态事件过滤器。
    void attachWindow();

    /// @brief 所在窗口指针，可能为 NULL（未进入顶层窗口前）。
    QWidget* m_boundWindow = nullptr;
    QLabel* m_logoLabel = nullptr;       ///< logo 标签，不取得所有权。
    QLabel* m_titleLabel = nullptr;      ///< 标题标签，不取得所有权。
    QPushButton* m_minimizeButton = nullptr; ///< 最小化按钮，不取得所有权。
    QPushButton* m_maximizeButton = nullptr; ///< 最大化 / 还原按钮，不取得所有权。
    QPushButton* m_closeButton = nullptr;    ///< 关闭按钮，不取得所有权。
    QPoint m_dragOffset;                 ///< 拖拽时鼠标相对窗口左上角的偏移。
    bool m_dragging = false;             ///< 是否正在拖拽标题条。
    int m_cornerRadius = 0;              ///< 调用方告知的窗口圆角半径（像素）。
};

#endif // CARBONUIQTITLEBAR_H
