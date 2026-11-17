/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIWidgetWithHdrBtn.h
 * @brief 带标题栏的容器控件：左上角标题 + 右上角图标按钮（深色分割线隔开）+ 内容区。
 * @author CarbonUI
 * @date  2026-11-16
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 控件外观参照 tools/reference/WidgetWithHeaderButton.png：外层为默认显示的
 * 2px #676767 边框（不依赖 objectName，调用方重命名后仍可见），顶部为略亮的
 * 标题栏，标题栏左侧为控件名称 QLabel，右侧为若干 24x24 图标按钮，
 * 相邻按钮之间插入 1px 深色竖向分割线。按钮点击后统一发出 `buttonClicked`
 * 信号，并把调用 `addButton` 时传入的 id 原样回传，由使用方决定具体行为。
 *
 * 内容区可通过 `contentWidget` 取得，使用方自行给它设置布局并放入子控件。
 * 本类只负责外观装配、分割线插入与点击信号转发，不解释按钮语义。
 *
 * 线程约定：与所有 QWidget 一样，只允许在 GUI 主线程构造与使用。
 */

#ifndef CARBONUIWIDGETWITHHDRBTN_H
#define CARBONUIWIDGETWITHHDRBTN_H

#include "carbonui_global.h"

#include <QtCore/QHash>
#include <QtWidgets/QWidget>

class QHBoxLayout;
class QIcon;
class QLabel;
class QPaintEvent;
class QString;
class QToolButton;

/**
 * @brief 带标题栏与图标按钮组的容器控件。
 */
class CARBONUI_DLL_API CarbonUIWidgetWithHdrBtn : public QWidget {
	Q_OBJECT

  public:
	/**
	 * @brief 构造控件并完成标题栏/内容区装配。
	 * @param[in] title  左上角显示的控件名称，可为空串，之后可用 @ref setTitle 修改。
	 * @param[in] parent 父控件指针，可为 nullptr。
	 */
	explicit CarbonUIWidgetWithHdrBtn(const QString& title = QString(), QWidget* parent = nullptr);

	/** @brief 设置左上角标题文本。 */
	void setTitle(const QString& title);

	/** @brief 查询当前左上角标题文本。 */
	QString title() const;

	/**
	 * @brief 在标题栏右侧追加一个图标按钮（非首按钮时先插入深色分割线）。
	 * @param[in] id       按钮标识，不可为空；点击时由 @ref buttonClicked 原样回传。
	 * @param[in] icon     按钮图标。
	 * @param[in] toolTip  鼠标悬停提示。
	 */
	void addButton(const QString& id, const QIcon& icon, const QString& toolTip);

	/** @brief 更新指定 id 按钮的悬停提示；id 不存在时静默返回。 */
	void setButtonToolTip(const QString& id, const QString& toolTip);

	/** @brief 查询已添加的图标按钮数量。 */
	int buttonCount() const;

	/** @brief 取得内容区容器，使用方可对其设置布局并放入任意子控件。 */
	QWidget* contentWidget() const;

  signals:
	/**
	 * @brief 某个标题栏图标按钮被点击。
	 * @param[in] id 该按钮在 @ref addButton 中登记的标识。
	 */
	void buttonClicked(const QString& id);

  protected:
	/**
	 * @brief 自绘默认边框：QSS 的 border 在自定义 QWidget 上不可靠，改为 QPainter 直绘。
	 */
	void paintEvent(QPaintEvent* event) override;

  private:
	void buildUi(); ///< 构建控件树与 QSS（仅构造时调用）。

	QLabel* titleLabel_ = nullptr;         ///< 左上角标题标签。
	QHBoxLayout* headerLayout_ = nullptr;  ///< 标题栏布局：标题 + 按钮/分割线。
	QWidget* content_ = nullptr;           ///< 内容区容器。
	QHash<QString, QToolButton*> buttons_; ///< id -> 图标按钮，便于重设 tooltip。
};

#endif // CARBONUIWIDGETWITHHDRBTN_H
