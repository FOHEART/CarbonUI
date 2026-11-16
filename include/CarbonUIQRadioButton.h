/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQRadioButton.h
 * @brief 分段单选按钮控件：构造即获得 CarbonUI 暗色单选按钮外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观与内部结构，构造函数内
 * 完成全部装配，调用方只需 new 后加入布局。
 *
 * 结构约定：原生 indicator 被压成 0x0，按钮内部创建名为
 * `<objectName>_label` 的子 QLabel 承载文字（构造后再调
 * @c QRadioButton::setText() 改文案即可，内部过滤器会在 paint / show /
 * resize / polish 等时机把文字同步回标签）。
 *
 * 外观契约：面板 \#272727、未选中文字 \#cdcdcd、选中底 \#009ee8 + 白字，
 * 四角圆角由 ::CarbonUIQRadioButton::RadioShape 决定。
 *
 * 需要给"外部已有的 QRadioButton"刷同样外观的调用方，可改用静态接口
 * CarbonUIQRadioButton::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQRADIOBUTTON_H
#define CARBONUIQRADIOBUTTON_H

#include "carbonui_global.h"

#include <QtCore/QString>
#include <QtWidgets/QRadioButton>

/**
 * @brief CarbonUI 分段单选按钮：自带暗色 QSS 与文字容器约定的 QRadioButton 子类。
 *
 * @details
 * 圆角组合（10px / 0）专为"多个单选按钮拼成一条分段选择器"设计；数值本身
 * 不参与运算，仅作稳定标识，不保证大小顺序。
 */
class CARBONUI_DLL_API CarbonUIQRadioButton : public QRadioButton {
  public:
	/**
	 * @brief 单选按钮外形枚举（含义见类说明）。
	 */
	enum RadioShape {
		Radio_FourRound = 0, ///< 四角均为 12px 圆角，独立按钮使用。
		Radio_LeftRound,     ///< 左侧两角 10px、右侧两角 0，分段选择器最左一格。
		Radio_RightRound,    ///< 右侧两角 10px、左侧两角 0，分段选择器最右一格。
		Radio_Rectangle      ///< 四角均为 0，分段选择器中间格。
	};

	/**
	 * @brief 构造单选按钮并应用 @p shape 外形。
	 *
	 * @param[in] text     初始文案，可为空串；非空时写入内部文字容器。
	 * @param[in] parent   父控件指针，**可为 NULL**。
	 * @param[in] shape    外形，取值必须是 ::CarbonUIQRadioButton::RadioShape 中
	 *                     的枚举值；传入其他值按 ::Radio_FourRound 处理。
	 * @param[in] minWidth 最小宽度，单位像素，取值 [0, INT_MAX]，原样写入 QSS
	 *                     的 @c min-width；负值会被 Qt 当作无效长度忽略。
	 */
	explicit CarbonUIQRadioButton(const QString& text = QString(), QWidget* parent = nullptr,
	                              RadioShape shape = Radio_FourRound, int minWidth = 28);

	/**
	 * @brief 切换外形与最小宽度并重刷样式表。
	 * @param[in] shape    新外形，语义同构造函数的 @p shape。
	 * @param[in] minWidth 新最小宽度，单位像素，语义同构造函数的 @p minWidth。
	 */
	void setRadioShape(RadioShape shape, int minWidth);

	/**
	 * @brief 查询当前外形。
	 * @return 构造或最近一次 @c setRadioShape() 设置的外形。
	 */
	RadioShape radioShape() const;

	/**
	 * @brief 给已有单选按钮应用 CarbonUI 外观（不改变对象类型）。
	 *
	 * @param[in,out] rb       目标单选按钮，**可为 NULL**，为 NULL 时直接返回。
	 * @param[in]     shape    外形，语义同构造函数的 @p shape；默认
	 *                         ::CarbonUIQRadioButton::Radio_FourRound。
	 * @param[in]     minWidth 最小宽度，语义同构造函数的 @p minWidth；默认 28。
	 * @note 本类构造函数内部即调用 @c applyStyle(this, shape, minWidth)。
	 */
	static void applyStyle(QRadioButton* rb, RadioShape shape = Radio_FourRound, int minWidth = 28);

  private:
	RadioShape m_shape; ///< 当前外形。
};

#endif // CARBONUIQRADIOBUTTON_H
