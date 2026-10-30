/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  groupboxcollapser.h
 * @brief 把普通 QGroupBox 改造为可动画折叠面板的控制器。
 * @author heqingrui
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui
 *
 * @details
 * 本头文件声明 @ref QGroupBoxCollapser 。它**不是**控件，而是一个挂在目标
 * QGroupBox 上的 QObject 控制器：安装时把分组框原有的布局搬进内部
 * `QGroupBoxBody` 子控件，把标题搬进布局的 menuBar 槽里的可勾选 QToolButton，
 * 并在折叠/展开时用 180ms 的 `maximumHeight` 属性动画完成过渡。
 *
 * 生命周期：控制器以分组框为 QObject 父对象，分组框销毁时一并销毁；
 * 调用方只需保留 `installIntoGroupBox()` 的返回值以调用
 * @ref QGroupBoxCollapser::addWidget 等方法，**不要**自行 delete。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef QGROUPBOXCOLLAPSER_H
#define QGROUPBOXCOLLAPSER_H

#include "carbonui_global.h"

#include <QObject>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QWidget>
#include <QtCore/QPropertyAnimation>

/**
 * @brief 分组框折叠控制器：接管标题栏，并为内容区提供下拉/收起动画。
 *
 * @details
 * 安装完成后分组框的结构变成：
 * @code
 * QGroupBox
 *  └─ QVBoxLayout (由控制器创建，menuBar 槽里放着 "QGroupBoxTitle")
 *      └─ QWidget "QGroupBoxBody"   <- 原分组框布局与原有子控件都在这里
 *          └─ 原 QVBoxLayout / 原控件
 * @endcode
 *
 * 分组框自身的 `title()` 会被清空，标题文字改由 QToolButton 显示；控制器在
 * 分组框上安装事件过滤器，在 LayoutRequest / Paint / Polish 等时机把
 * `groupBox->setTitle()` 的新值同步到按钮上，因此调用方可以继续用
 * `setTitle()` 改标题，不必知道内部结构。
 */
class CARBONUI_DLL_API QGroupBoxCollapser : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 就地把一个已填充内容的 QGroupBox 改造成可折叠面板。
     *
     * @details
     * 执行顺序：改写分组框样式表 -> 创建内容区并接管原布局 -> 创建标题行与
     * 可勾选按钮 -> 复制分组框原有样式表到新控件 -> 建立 180ms 动画。
     * 传入 @p expanded = false 时会额外把按钮置为未选中、箭头朝右并隐藏内容区。
     *
     * @param[in,out] groupBox     目标分组框，**不可为 NULL**；必须是已在
     *                             heap 上创建且**已设置布局**的实例，
     *                             否则布局搬迁会失败（Qt 会打印警告）。
     * @param[in]     cornerWidget 标题行右侧的附加控件（如"编辑"按钮），
     *                             **可为 NULL**；为 NULL 时不加任何控件。
     *                             传入后其父对象会被改成标题行控件。
     * @param[in]     expanded     初始是否展开，默认 false（折叠）。
     * @return 新建的控制器指针，父对象为 @p groupBox，取值一定非 NULL；
     *         随 @p groupBox 一起销毁，调用方不要 delete。
     * @pre @p groupBox 非 NULL 且已通过 `setLayout()` 安装过布局。
     */
    static QGroupBoxCollapser *installIntoGroupBox(QGroupBox *groupBox, QWidget *cornerWidget = nullptr, bool expanded = false);

    /**
     * @brief 把一个控件追加到可折叠内容区。
     *
     * @details
     * 优先追加到内容区现有布局的末尾；内容区没有布局时改为直接把控件
     * reparent 到内容区（由控件自身的几何决定位置）。
     *
     * @param[in] widget 待添加的控件，**可为 NULL**，为 NULL 时直接返回；
     *                   非 NULL 时其所有权转交给内容区（成为子控件）。
     * @note 不影响动画：展开过程中新增的控件会在下次展开时按新的 sizeHint 计算高度。
     */
    void addWidget(QWidget *widget);

protected:
    /**
     * @brief 监听分组框的标题变化信号，把 `title()` 同步到标题按钮。
     *
     * @details
     * 只在 @p watched 是宿主分组框、且事件类型属于布局/绘制/语言切换类时
     * 触发同步；其余事件与对象一律原样交给基类处理。
     *
     * @param[in] watched 事件接收者，可为任意 QObject。
     * @param[in] event   事件对象，不可为 NULL。
     * @return 恒定返回 false（不拦截事件，只做副作用）。
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    /**
     * @brief 构造函数，仅由 @ref installIntoGroupBox 调用。
     *
     * @param[in] groupBox     被接管的分组框，不可为 NULL，且必须已设置布局。
     * @param[in] cornerWidget 标题行右侧附加控件，可为 NULL。
     */
    QGroupBoxCollapser(QGroupBox *groupBox, QWidget *cornerWidget = nullptr);

    /**
     * @brief 按钮点击后的折叠/展开逻辑：切换箭头并启动高度动画。
     *
     * @param[in] checked `QAbstractButton::clicked` 携带的勾选状态；
     *                    函数内以 `m_toggleButton->isChecked()` 为准。
     */
    void toggle(bool checked);

    /**
     * @brief 把 `groupBox->title()` 搬到标题按钮并清空分组框标题。
     *
     * @details 标题为空、或控件尚未就绪时不改动任何状态。
     */
    void syncTitleFromGroupBox();

private slots:
    /**
     * @brief 动画结束后的收尾：折叠则隐藏内容区，展开则解除高度限制。
     */
    void onAnimationFinished();

private:
    QGroupBox* m_groupBox;            ///< 被接管的分组框，不可为 NULL。
    QWidget *m_titleWidget;           ///< 标题行控件，objectName 为 "QGroupBoxTitle"。
    QToolButton *m_toggleButton;      ///< 标题栏里的可勾选按钮，承载标题文字与箭头。
    QWidget *m_contentArea;           ///< 内容区控件，objectName 为 "QGroupBoxBody"。
    QPropertyAnimation *m_animation;  ///< 作用于内容区 `maximumHeight` 的 180ms 动画。
    bool m_collapsing;                ///< true 表示当前动画方向是折叠，供收尾槽判断。
};

#endif // QGROUPBOXCOLLAPSER_H
