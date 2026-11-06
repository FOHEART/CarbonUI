/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass.h
 * @brief CarbonUI 样式助手门面：把统一的暗色 QSS 绑定到 Qt 控件。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 本头文件只声明无状态静态门面 @ref CarbonUIClass 。每个 `applyXxxStyle` /
 * `setupXxxStyle` 接收一个 Qt 控件指针，写入一段完整的 QSS 字符串，并按需
 * 连带修改几何、字体、光标与校验器；接口之间互不依赖，可以按需单独调用。
 *
 * 线程约定：所有接口都会触碰 QWidget 实例，只允许在 GUI 主线程调用。
 * 错误处理：控件指针为 NULL 时静默返回，调用方无需判空。
 */

#ifndef CARBONUICLASS_H
#define CARBONUICLASS_H

#include <QtWidgets/QRadioButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QStatusBar>

#include "carbonui_global.h"

/**
 * @brief 控件样式助手集合（全静态、无数据成员）。
 *
 * @details
 * 用法是"拿到控件就调一次"，不返回对象；少数接口会在控件内部补一个子控件
 * （单选按钮的 `<objectName>_label`、复选框的 `<objectName>_indicator`），
 * 详见各接口说明。配色属于项目调色板：强调色 \#009ee8（悬停 \#00b0ff、
 * 按下 \#007bb3）、面板 \#272727 / \#2e2e2e / \#2f2f2f / \#3b3b3b、
 * 正文 \#cdcdcd / \#ffffff、次级文字 \#676767、分隔线 \#5f5f5f / \#636363。
 *
 * 线程安全：非线程安全，只能在 GUI 主线程使用。
 */
class CARBONUI_DLL_API CarbonUIClass
{
public:
    /**
     * @brief 单选按钮外形枚举。
     *
     * @details
     * 取值只决定四角圆角半径的组合，用于把多个单选按钮拼成一条分段选择器；
     * 数值本身不参与运算，仅作稳定标识，不保证大小顺序。
     */
    enum RadioShape
    {
        Radio_FourRound = 0, ///< 四角均为 12px 圆角，独立按钮使用。
        Radio_LeftRound,     ///< 左侧两角 10px、右侧两角 0，分段选择器最左一格。
        Radio_RightRound,    ///< 右侧两角 10px、左侧两角 0，分段选择器最右一格。
        Radio_Rectangle      ///< 四角均为 0，分段选择器中间格。
    };

    /**
     * @brief 给整个应用补一条 `QToolTip` 样式（深底、蓝字、2px 边框）。
     *
     * @details
     * 实现是"追加"而不是"替换"：把 `QToolTip` 规则拼接到
     * `qApp->styleSheet()` 末尾，因此重复调用会重复追加同样的规则。
     * `qApp` 为 NULL（尚无 QApplication）时静默返回。
     *
     * @note 这是本类唯一修改全局状态的接口，会作用于所有控件。
     */
    static void applyTooltipStyle();

    /**
     * @brief 应用单选按钮外观，并把文字搬进居中的子标签。
     *
     * @details
     * 原生 indicator 被压成 0x0，随后创建（或复用）名为
     * `<objectName>_label` 的子 QLabel 承载文字，并把 `rb->text()` 清空。
     * 之后要改文字请继续调用 `QRadioButton::setText()`，隐藏的事件过滤器会在
     * paint / show / resize / polish 等时机把文字同步回标签。
     * 重复调用是幂等的：不会重复创建标签，也不会重复安装过滤器。
     *
     * @param[in,out] rb       目标单选按钮，**可为 NULL**，为 NULL 时直接返回。
     * @param[in]     shape    外形，取值必须是 ::CarbonUIClass::RadioShape 中的枚举值；
     *                         传入其他值按 `::CarbonUIClass::Radio_FourRound` 处理。
     *                         默认 `::CarbonUIClass::Radio_FourRound`。
     * @param[in]     minWidth 最小宽度，单位像素，取值 [0, INT_MAX]，
     *                         原样写入 QSS 的 `min-width`；默认 28。
     *                         负值不会被拒绝，但会被 Qt 当作无效长度忽略。
     */
    static void applyRadioStyle(QRadioButton* rb, RadioShape shape = Radio_FourRound, int minWidth = 28);

    /**
     * @brief 应用"数字输入框"外观：居中文本、聚焦全选、C 语言环境数值校验。
     *
     * @details
     * 除 QSS 外还会：设置 IBeam 光标、零内容边距、居中对齐、安装
     * `SelectAllOnFocusFilter`（获得焦点或用鼠标按下时全选文本）、连接
     * `returnPressed` 到 `clearFocus`，并安装一个范围 [-1e12, 1e12]、6 位小数、
     * 仅标准计数法（不接受 `1e5`）且固定 C 语言环境的 QDoubleValidator。
     *
     * @param[in,out] le    目标单行输入框，**可为 NULL**，为 NULL 时直接返回。
     * @param[in]     width 固定宽度，单位像素，取值 [0, INT_MAX]；
     *                      0 表示不固定宽度（保持布局决定的自然宽度），默认 0。
     * @note 每次调用都会新建一个事件过滤器与校验器（父对象为该输入框）。
     */
    static void applyLineEditStyle_numberInput(QLineEdit* le, int width = 0);

    /**
     * @brief 应用"自绘指示灯"复选框外观。
     *
     * @details
     * 原生 indicator 被压成 0x0，按钮固定为 16x16，内部新增 8x8 的圆点子控件
     * `<objectName>_indicator`（透明接收鼠标事件）；`text()` 被清空。
     * 复选框的勾选状态、悬停高亮与左键单击切换都由本函数安装的事件过滤器
     * 维护，因此样式生效后**不要**再给该控件装自定义 mousePress 逻辑。
     *
     * @param[in,out] cb 目标复选框，**可为 NULL**，为 NULL 时直接返回。
     * @warning 非幂等：重复调用会再建一个指示灯子控件与布局。
     */
    static void applyCheckStyle(QCheckBox* cb);

    /**
     * @brief 应用无边框滚动区外观，并把纵向滚动条收窄到 4px。
     *
     * @param[in,out] area 目标滚动区，**可为 NULL**，为 NULL 时直接返回。
     */
    static void applyScrollAreaStyle(QScrollArea* area);

    /**
     * @brief 按钮外形枚举。
     *
     * @details
     * 用"只圆左半边 / 只圆右半边"的按钮拼出分段的工具栏按钮组；
     * 数值本身不参与运算，仅作稳定标识。
     */
    enum ButtonShape
    {
        Shape_RoundAll = 0, ///< 四角 10px 圆角，独立按钮使用。
        Shape_LeftRound,    ///< 左两角 10px、右两角 0，分段按钮组最左一格。
        Shape_Rectangle,    ///< 四角 0，分段按钮组中间格。
        Shape_RightRound    ///< 右两角 10px、左两角 0，分段按钮组最右一格。
    };

    /**
     * @brief 应用主色按钮外观，按形状决定圆角。
     *
     * @param[in,out] btn   目标按钮，**可为 NULL**，为 NULL 时直接返回。
     * @param[in]     shape 外形，取值必须是 ::CarbonUIClass::ButtonShape 中的枚举值；
     *                      传入其他值时不设置任何圆角（等效于 0px）。
     *                      默认 `::CarbonUIClass::Shape_RoundAll`。
     */
    static void applyButtonStyle(QPushButton* btn, ButtonShape shape = Shape_RoundAll);

    /**
     * @brief 应用窗口菜单栏外观（关闭原生菜单栏、10pt 加粗、固定 26px 高）。
     *
     * @param[in,out] b 目标菜单栏，**可为 NULL**，为 NULL 时直接返回。
     */
    static void setupMenuBarStyle(QMenuBar* b);

    /**
     * @brief 应用工具栏外观：禁止拖动/浮动、固定 32px 上限高、28px 图标。
     *
     * @param[in,out] tb 目标工具栏，**可为 NULL**，为 NULL 时直接返回。
     * @note `setIconSize()` 会清掉 `setFixedHeight()` 设置的最小高度，
     *       因此高度契约只体现在 `maximumHeight() == 32` 上。
     */
    static void setupToolBarStyle(QToolBar* tb);

    /**
     * @brief 应用标签页外观（面板底色、选中页签高亮、右侧预留 66px 滚动区）。
     *
     * @param[in,out] tw 目标标签页控件，**可为 NULL**，为 NULL 时直接返回。
     */
    static void setupTabWidgetStyle(QTabWidget* tw);

    /**
     * @brief 应用下拉框外观（圆角输入框 + 资源图片箭头）。
     *
     * @param[in,out] cb 目标下拉框，**可为 NULL**，为 NULL 时直接返回。
     * @note 箭头图片来源为 `:/img/Resources/img/arrow_down.png`，
     *       依赖 `Resource.qrc` 的前缀 `/img`。
     */
    static void setupComboBoxStyle(QComboBox* cb);

    /**
     * @brief 应用状态栏外观（深色底 + 次级文字色）。
     *
     * @param[in,out] sb 目标状态栏，**可为 NULL**，为 NULL 时直接返回。
     */
    static void setupQStatusBarStyle(QStatusBar* sb);

    /**
     * @brief 应用表格型树控件外观：压平缩进、加粗表头。
     *
     * @details
     * 设置 `indentation = 0`、关闭根节点装饰，表头左对齐且最小高度 24px，
     * 表头用 13pt 加粗字体，正文用 13pt 常规字体。
     *
     * @param[in,out] tree 目标树控件，**可为 NULL**，为 NULL 时直接返回。
     */
    static void applyTreeWidgetStyle(QTreeWidget* tree);

    /**
     * @brief 应用层级型树视图外观：保留 14px 缩进与展开箭头，禁止编辑。
     *
     * @param[in,out] view 目标树视图，**可为 NULL**，为 NULL 时直接返回；
     *                     可传入 QTreeWidget 等子类实例。
     * @note 展开/收起箭头的图片路径为 `:/images/...`，与 `Resource.qrc` 的
     *       `/img` 前缀不一致，资源不存在时箭头不显示（不影响其它属性）。
     */
    static void applyTreeViewStyle(QTreeView* view);

    /**
     * @brief 递归地把所有后代 QLabel 的文字颜色改成次级色 \#676767。
     *
     * @details
     * 只处理 QLabel，其它控件（含 QPushButton 等）的样式表保持原样；
     * 已有样式表会被整体覆盖为 `color: \#676767;`。
     *
     * @param[in,out] w 遍历起点，**可为 NULL**，为 NULL 时直接返回。
     */
    static void setWidgetLabelsStyleRecursively(QWidget* w);


private:
    /// @name 兼容旧调用点的包装
    /// @{

    /**
     * @brief 等价于 `applyRadioStyle(rb, Radio_LeftRound)`。
     * @param[in,out] rb 目标单选按钮，可为 NULL。
     */
    static inline void applyRadioStyleLeftRound(QRadioButton* rb) { applyRadioStyle(rb, Radio_LeftRound); }

    /**
     * @brief 等价于 `applyRadioStyle(rb, Radio_RightRound)`。
     * @param[in,out] rb 目标单选按钮，可为 NULL。
     */
    static inline void applyRadioStyleRightRound(QRadioButton* rb) { applyRadioStyle(rb, Radio_RightRound); }

    /**
     * @brief 等价于 `applyRadioStyle(rb, Radio_Rectangle)`。
     * @param[in,out] rb 目标单选按钮，可为 NULL。
     */
    static inline void applyRadioStyleRectangle(QRadioButton* rb) { applyRadioStyle(rb, Radio_Rectangle); }

    /**
     * @brief 等价于 `applyRadioStyle(rb, Radio_FourRound)`。
     * @param[in,out] rb 目标单选按钮，可为 NULL。
     */
    static inline void applyRadioStyleFourRoundCorner(QRadioButton* rb){ applyRadioStyle(rb, Radio_FourRound); }

    /// @}
};

#endif // CARBONUICLASS_H
