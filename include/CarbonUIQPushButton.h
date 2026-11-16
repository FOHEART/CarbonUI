/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQPushButton.h
 * @brief 主色按钮控件：构造即获得 CarbonUI 暗色按钮外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS、
 * 光标与形状设置，调用方只需 new 后加入布局。
 *
 * 外观契约：强调色 \#009ee8（悬停 \#00b0ff、按下 \#007bb3），禁用态独立配色
 * （\#636363 底、\#cfcfcf 字）；四角圆角由 ::CarbonUIQPushButton::ButtonShape
 * 决定，可用 @c setButtonShape() 随时切换。
 *
 * 需要给"外部已有的 QPushButton"刷同样外观的调用方，可改用静态接口
 * CarbonUIQPushButton::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQPUSHBUTTON_H
#define CARBONUIQPUSHBUTTON_H

#include "carbonui_global.h"

#include <QtCore/QString>
#include <QtWidgets/QPushButton>

/**
 * @brief CarbonUI 主色按钮：自带暗色 QSS 的 QPushButton 子类。
 *
 * @details
 * 圆角组合（10px / 0）专为"多个按钮拼成一条分段选择器"设计：最左格
 * ::CarbonUIQPushButton::Shape_LeftRound、中间格 ::Shape_Rectangle、最右格
 * ::Shape_RightRound、独立按钮 ::Shape_RoundAll。数值本身不参与运算，仅作
 * 稳定标识，不保证大小顺序。
 */
class CARBONUI_DLL_API CarbonUIQPushButton : public QPushButton {
  public:
	/**
	 * @brief 按钮外形枚举（含义见类说明）。
	 */
	enum ButtonShape {
		Shape_RoundAll = 0, ///< 四角 10px 圆角，独立按钮使用。
		Shape_LeftRound,    ///< 左两角 10px、右两角 0，分段按钮组最左一格。
		Shape_Rectangle,    ///< 四角 0，分段按钮组中间格。
		Shape_RightRound    ///< 右两角 10px、左两角 0，分段按钮组最右一格。
	};

	/**
	 * @brief 构造按钮并应用 @p shape 外形。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 * @param[in] shape  外形，取值必须是 ::CarbonUIQPushButton::ButtonShape 中的
	 *                   枚举值；传入其他值时不设置任何圆角（等效于 0px）。
	 */
	explicit CarbonUIQPushButton(QWidget* parent = nullptr, ButtonShape shape = Shape_RoundAll);

	/**
	 * @brief 构造带文字的按钮并应用 @p shape 外形。
	 * @param[in] text   按钮文字，可为空串。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 * @param[in] shape  外形，语义同上一构造函数。
	 */
	CarbonUIQPushButton(const QString& text, QWidget* parent = nullptr,
	                    ButtonShape shape = Shape_RoundAll);

	/**
	 * @brief 切换外形并重刷样式表。
	 * @param[in] shape 新外形，语义同构造函数的 @p shape。
	 */
	void setButtonShape(ButtonShape shape);

	/**
	 * @brief 查询当前外形。
	 * @return 构造或最近一次 @c setButtonShape() 设置的外形。
	 */
	ButtonShape buttonShape() const;

	/**
	 * @brief 给已有按钮应用 CarbonUI 外观（不改变对象类型）。
	 *
	 * @param[in,out] btn   目标按钮，**可为 NULL**，为 NULL 时直接返回。
	 * @param[in]     shape 外形，语义同构造函数的 @p shape；默认
	 *                      ::CarbonUIQPushButton::Shape_RoundAll。
	 * @note 本类构造函数内部即调用 @c applyStyle(this, shape)。
	 */
	static void applyStyle(QPushButton* btn, ButtonShape shape = Shape_RoundAll);

  private:
	ButtonShape m_shape; ///< 当前外形。
};

#endif // CARBONUIQPUSHBUTTON_H
