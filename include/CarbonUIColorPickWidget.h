/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIColorPickWidget.h
 * @brief 颜色选择控件：色相/饱和度选色区 + 明度滑条 + RGB/十六进制/透明度输入。
 * @author CarbonUI
 * @date  2026-11-17
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 复刻参考界面（tools/reference/ColorPickWidget.png）的深色取色面板：左侧为
 * 色相（横向）× 饱和度（纵向，底部白色，恒以满明度绘制）选色区，其右为纵向
 * 明度滑条（上白下黑），右列自上而下为 Ok / Cancel 按钮、R/G/B 输入框、
 * 十六进制输入框、透明度（百分比）输入框与当前颜色预览块。所有子控件由本类
 * 在构造函数内装配并应用 CarbonUI 暗色 QSS，调用方只需构造 + 取值/选色即可，
 * 无需关心内部结构。
 *
 * 与 CarbonUIQ* 控件类同一实现范式：外观与结构都由控件自己负责。选色区与
 * 明度滑条为纯手绘（QPainter 渐变），鼠标按下/拖动即取色并实时回填右侧输入。
 *
 * 信号：任何来源（选色、滑条、RGB、十六进制、透明度）改变当前颜色都会发
 * colorChanged 信号；Ok / Cancel 按钮分别发 accepted / rejected 信号，
 * 直接放进 QDialog 时可经这两个信号驱动 accept() / reject()。
 *
 * 用法示例：
 * @code
 * auto* picker = new CarbonUIColorPickWidget(parent);
 * picker->setColor(QColor(0x00, 0x9e, 0xe8));
 * connect(picker, &CarbonUIColorPickWidget::accepted, dialog, &QDialog::accept);
 * connect(picker, &CarbonUIColorPickWidget::rejected, dialog, &QDialog::reject);
 * @endcode
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程使用。
 */

#ifndef CARBONUICOLORPICKWIDGET_H
#define CARBONUICOLORPICKWIDGET_H

#include "carbonui_global.h"

#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

/**
 * @brief CarbonUI 颜色选择控件：自带暗色 QSS 的完整取色面板。
 */
class CARBONUI_DLL_API CarbonUIColorPickWidget : public QWidget {
	Q_OBJECT

  public:
	/**
	 * @brief 构造取色面板并完成外观与子控件装配，初始颜色为黑色。
	 * @param[in] parent 父控件指针，**可为 NULL**。
	 */
	explicit CarbonUIColorPickWidget(QWidget* parent = nullptr);

	/**
	 * @brief 查询当前选中的颜色（含透明度通道）。
	 * @return 当前颜色；透明度取值映射自透明度输入框的百分比。
	 */
	QColor color() const;

  public slots:
	/**
	 * @brief 设置当前颜色并同步全部子控件（选色区光标、滑条、RGB、十六进制、透明度）。
	 * @param[in] color 目标颜色，可为无效颜色（此时按黑色处理）。
	 * @note 不发射 colorChanged 信号，与用户交互路径区分开。
	 */
	void setColor(const QColor& color);

  signals:
	/**
	 * @brief 当前颜色因用户交互而变化。
	 * @param[in] color 新颜色（含透明度通道）。
	 */
	void colorChanged(const QColor& color);

	/**
	 * @brief 点击了 Ok 按钮。
	 */
	void accepted();

	/**
	 * @brief 点击了 Cancel 按钮。
	 */
	void rejected();

  private:
	/// @brief 装配子控件、布局与样式（仅构造时调用一次）。
	void buildUi();

	/// @brief 把当前颜色回填到 RGB / 十六进制 / 透明度输入框（不触发回环）。
	void syncEditors();

	/**
	 * @brief 选色区取色回调：按色相/饱和度更新颜色并通知。
	 * @param[in] hue        色相 [0, 1]。
	 * @param[in] sat        饱和度 [0, 1]。
	 * @param[in] val        明度 [0, 1]（来自明度滑条的当前值）。
	 * @param[in] emitChange true 时发射 colorChanged 信号。
	 */
	void applyHsv(double hue, double sat, double val, bool emitChange);

	/**
	 * @brief RGB / 十六进制 / 透明度输入回调：按输入更新颜色并同步选色区。
	 * @param[in] emitChange true 时发射 colorChanged 信号。
	 */
	void applyEditorChange(bool emitChange);

	QColor color_ = Qt::black; ///< 当前颜色（含透明度通道）。
	class ColorAreaWidget*
	    area_; ///< 色相×饱和度选色区（实现见 .cpp），不取得所有权语义之外的生命周期。
	class ValueSliderWidget* slider_; ///< 纵向明度滑条（实现见 .cpp）。
	QSpinBox* spinR_ = nullptr;       ///< R 通道输入框（objectName: carbonUIColorPickSpinR）。
	QSpinBox* spinG_ = nullptr;       ///< G 通道输入框（objectName: carbonUIColorPickSpinG）。
	QSpinBox* spinB_ = nullptr;       ///< B 通道输入框（objectName: carbonUIColorPickSpinB）。
	QSpinBox* spinA_ = nullptr; ///< 透明度（百分比）输入框（objectName: carbonUIColorPickSpinA）。
	QLineEdit* hexEdit_ = nullptr;    ///< 十六进制输入框（objectName: carbonUIColorPickHex）。
	QLabel* previewLabel_ = nullptr;  ///< 当前颜色预览块（objectName: carbonUIColorPickPreview）。
	QPushButton* okButton_ = nullptr; ///< Ok 按钮（objectName: carbonUIColorPickOk）。
	QPushButton* cancelButton_ = nullptr; ///< Cancel 按钮（objectName: carbonUIColorPickCancel）。
	bool updating_ = false;               ///< 程序化回填输入框期间为 true，避免信号回环。
};

#endif // CARBONUICOLORPICKWIDGET_H
