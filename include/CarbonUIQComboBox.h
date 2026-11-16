/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIQComboBox.h
 * @brief 下拉框控件：构造即获得 CarbonUI 暗色下拉框外观。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 与 CarbonUIDialog 同一实现范式：控件自己负责外观，构造函数内完成全部 QSS
 * 装配，调用方只需 new 后加入布局并 addItem()。
 *
 * 外观契约：圆角输入框（面板 \#2f2f2f、文字 \#cdcdcd、边框 \#5f5f5f），
 * 下拉区右侧 28px 分隔栏，箭头使用资源图片。
 *
 * @note 箭头图片来源为 `:/img/Resources/img/arrow_down.png`，依赖
 *       `carbonui_rcc.qrc` 的 `/img` 前缀。
 *
 * 需要给"外部已有的 QComboBox"刷同样外观的调用方，可改用静态接口
 * CarbonUIQComboBox::applyStyle()（构造函数内部即调用它）。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUIQCOMBOBOX_H
#define CARBONUIQCOMBOBOX_H

#include "carbonui_global.h"

#include <QtWidgets/QComboBox>

/**
 * @brief CarbonUI 下拉框：自带暗色 QSS 的 QComboBox 子类。
 */
class CARBONUI_DLL_API CarbonUIQComboBox : public QComboBox {
  public:
	/**
	 * @brief 构造下拉框并完成外观装配。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIQComboBox(QWidget* parent = nullptr);

	/**
	 * @brief 给已有下拉框应用 CarbonUI 外观（不改变对象类型）。
	 * @param[in,out] cb 目标下拉框，**可为 NULL**，为 NULL 时直接返回。
	 * @note 本类构造函数内部即调用 @c applyStyle(this)。
	 */
	static void applyStyle(QComboBox* cb);
};

#endif // CARBONUIQCOMBOBOX_H
