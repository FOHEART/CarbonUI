/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQLineEdit.h
 * @brief 数字输入框控件：构造即获得 CarbonUI 暗色输入框外观与数值校验。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观与交互，构造函数内完成
 * 全部装配，调用方只需 new 后加入布局。
 *
 * 外观与行为契约：圆角输入框（面板 \#2f2f2f、文字 \#cdcdcd、边框 \#5f5f5f）、
 * IBeam 光标、零内容边距、居中对齐；获得焦点或鼠标按下时全选文本；回车收起
 * 焦点；安装范围 [-1e12, 1e12]、6 位小数、仅标准计数法（不接受 `1e5`）且
 * 固定 C 语言环境的 QDoubleValidator。
 *
 * 需要给"外部已有的 QLineEdit"刷同样外观的调用方，可改用静态接口
 * CarbonUIQLineEdit::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQLINEEDIT_H
#define CARBONUIQLINEEDIT_H

#include "carbonui_global.h"

#include <QtWidgets/QLineEdit>

/**
 * @brief CarbonUI 数字输入框：自带暗色 QSS、聚焦全选与数值校验的
 *        QLineEdit 子类。
 */
class CARBONUI_DLL_API CarbonUIQLineEdit : public QLineEdit {
  public:
	/**
	 * @brief 构造数字输入框并完成外观与校验装配。
	 *
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 * @param[in] width  固定宽度，单位像素，取值 [0, INT_MAX]；0 表示不固定
	 *                   宽度（保持布局决定的自然宽度）。负值不会被拒绝，但
	 *                   会被 Qt 当作无效长度忽略。
	 */
	explicit CarbonUIQLineEdit(QWidget* parent = nullptr, int width = 0);

	/**
	 * @brief 查询固定宽度。
	 * @return 构造时设置的固定宽度；0 表示未固定。
	 */
	int fixedWidth() const;

	/**
	 * @brief 给已有输入框应用 CarbonUI 数字输入框外观（不改变对象类型）。
	 *
	 * @param[in,out] le    目标单行输入框，**可为 NULL**，为 NULL 时直接返回。
	 * @param[in]     width 固定宽度，语义同构造函数的 @p width；默认 0。
	 * @note 本类构造函数内部即调用 @c applyStyle(this, width)。
	 */
	static void applyStyle(QLineEdit* le, int width = 0);

  private:
	int m_fixedWidth; ///< 构造时选定的固定宽度（0 表示未固定）。
};

#endif // CARBONUIQLINEEDIT_H
