/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQCheckBox.h
 * @brief 自绘指示灯复选框控件：构造即获得 CarbonUI 暗色复选框外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观与交互，构造函数内完成
 * 全部装配，调用方只需 new 后加入布局。
 *
 * 结构约定：原生 indicator 被压成 0x0，勾选状态、悬停高亮与左键单击切换由
 * 内部事件过滤器维护，因此样式生效后**不要**再给该控件装自定义 mousePress
 * 逻辑。指示灯在文字的哪一侧由 ::CarbonUIQCheckBox::CheckIndicatorPosition
 * 决定：
 * - ::Check_IndicatorLeft：按钮固定为 16x16 的裸圆点，文字由调用方旁置；
 * - ::Check_IndicatorRight：文字搬进按钮内部左侧的 `<objectName>_label`
 *   子标签（之后改文字仍走 @c QCheckBox::setText()，过滤器会同步），
 *   16x16 圆环子控件 `<objectName>_indicator` 及其内部 8x8 圆点靠右放置。
 *
 * 需要给"外部已有的 QCheckBox"刷同样外观的调用方，可改用静态接口
 * CarbonUIQCheckBox::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQCHECKBOX_H
#define CARBONUIQCHECKBOX_H

#include "carbonui_global.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidget>

/**
 * @brief CarbonUI 自绘指示灯复选框：自带暗色 QSS 与事件过滤器交互的
 *        QCheckBox 子类。
 */
class CARBONUI_DLL_API CarbonUIQCheckBox : public QCheckBox {
  public:
	/**
	 * @brief 复选框指示灯位置枚举（含义见类说明）。
	 */
	enum CheckIndicatorPosition {
		Check_IndicatorLeft = 0, ///< 圆点在左侧：按钮固定 16x16，文字由调用方旁置（旧形态）。
		Check_IndicatorRight     ///< 圆点在右侧：文字搬进按钮内部左对齐，多个复选框上下
		                         ///  排列时圆点右对齐。
	};

	/**
	 * @brief 构造复选框并应用 @p position 形态。
	 *
	 * @param[in] text     初始文案，可为空串；::Check_IndicatorRight 形态下写入
	 *                     内部文字容器，::Check_IndicatorLeft 形态下按钮文字
	 *                     清空（由调用方旁置文字）。
	 * @param[in] parent   父控件指针，**可为 NULL**。
	 * @param[in] position 指示灯位置，取值必须是
	 *                     ::CarbonUIQCheckBox::CheckIndicatorPosition 中的枚举值；
	 *                     传入其他值按 ::Check_IndicatorLeft 处理。
	 */
	explicit CarbonUIQCheckBox(const QString& text = QString(), QWidget* parent = nullptr,
	                           CheckIndicatorPosition position = Check_IndicatorLeft);

	/**
	 * @brief 查询当前指示灯位置形态。
	 * @return 构造时设置的形态。
	 */
	CheckIndicatorPosition indicatorPosition() const;

	/**
	 * @brief 给已有复选框应用 CarbonUI 外观（不改变对象类型）。
	 *
	 * @param[in,out] cb       目标复选框，**可为 NULL**，为 NULL 时直接返回。
	 * @param[in]     position 指示灯位置，语义同构造函数的 @p position；默认
	 *                         ::CarbonUIQCheckBox::Check_IndicatorLeft。
	 * @warning 非幂等：重复调用会再建一套指示灯子控件与布局。
	 * @note 本类构造函数内部即调用 @c applyStyle(this, position)。
	 */
	static void applyStyle(QCheckBox* cb, CheckIndicatorPosition position = Check_IndicatorLeft);

  private:
	CheckIndicatorPosition m_position; ///< 构造时选定的指示灯位置形态。
};

/**
 * @brief CarbonUI 复选按钮组：若干"分段式复选框"（无指示灯、文字居中、
 *        可勾选高亮）拼成的独立按钮组控件。
 *
 * @details
 * 与 CarbonUIQPushButton 的分段按钮同一套外形契约（首段左两角 10px 圆角、
 * 末段右两角 10px 圆角、中间段直角，相邻分段之间固定留 2px 缝隙，无边框、
 * min-height 28px）；配色以 tools/reference/GroupCheckBox.png 为准：
 * 未选中 \#676767 灰底白字、选中 \#009ee8 蓝底白字（悬停各加一档提亮）。
 * 组内分段**互斥**（内部经 QButtonGroup exclusive 接管，单选语义）：点击
 * 未选中段会选中它并取消其余段，点击已选中段不发生任何变化；默认选中
 * 首段，构造完成后组内始终有且仅有一个选中项。
 * 交互由内部事件过滤器接管
 * （样式生效后**不要**再给分段装自定义 mousePress 逻辑）。
 *
 * 文字沿用复选框的 `<objectName>_label` 子标签约定：调用方仍用
 * @c QCheckBox::setText() 改文案，过滤器会在重绘/布局时机同步。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */
class CARBONUI_DLL_API CarbonUIQCheckButtonGroup : public QWidget {
  public:
	/**
	 * @brief 构造按钮组并装配 @p segmentCount 个分段。
	 * @param[in] segmentCount 分段数量，小于 1 按 1 处理；分段外形由位置
	 *                         决定（首段左圆 / 末段右圆 / 中段直角）。
	 * @param[in] parent   父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQCheckButtonGroup(int segmentCount, QWidget* parent = nullptr);

	/**
	 * @brief 统一命名按钮组及其全部分段。
	 *
	 * 分段与内部文字标签的 objectName 在构造时按组名派生，构造后再改组名
	 * 不会自动跟随；需要稳定命名（如测试查找）时用本接口一次改齐。
	 *
	 * @param[in] name 组 objectName；分段命名为 `<name>_segment<i>`。
	 */
	void setGroupObjectName(const QString& name);

	/**
	 * @brief 查询分段数量。
	 * @return 构造时设置的分段数量。
	 */
	int segmentCount() const;

	/**
	 * @brief 按序号取分段复选框。
	 * @param[in] index 从 0 起的分段序号（从左到右）。
	 * @return 对应分段；序号越界返回 NULL。
	 */
	QCheckBox* segment(int index) const;

  private:
	QList<QCheckBox*> m_segments; ///< 分段复选框，顺序从左到右。
};

#endif // CARBONUIQCHECKBOX_H
