/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass.cpp
 * @brief CarbonUIClass 中跨控件/全局样式助手的实现。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui
 *
 * @details
 * 本文件只保留作用于全局或不绑定单一控件类的样式助手：
 * - @ref CarbonUIClass::applyTooltipStyle：向 `qApp` 追加全局 QToolTip 规则
 *   （本类唯一修改全局状态的接口）；
 * - @ref CarbonUIClass::setWidgetLabelsStyleRecursively：递归改写后代
 *   QLabel 的文字颜色。
 *
 * 单控件助手按控件类拆分到同名子文件（与 CarbonUIClass_QRadioButton.cpp
 * 同一约定）：
 * - CarbonUIClass_QCheckBox.cpp   （applyCheckStyle 及其内部过滤器）
 * - CarbonUIClass_QLineEdit.cpp   （applyLineEditStyle_numberInput 及全选过滤器）
 * - CarbonUIClass_QPushButton.cpp （applyButtonStyle）
 * - CarbonUIClass_QScrollArea.cpp （applyScrollAreaStyle）
 * - CarbonUIClass_QMenuBar.cpp    （setupMenuBarStyle）
 * - CarbonUIClass_QToolBar.cpp    （setupToolBarStyle）
 * - CarbonUIClass_QTabWidget.cpp  （setupTabWidgetStyle）
 * - CarbonUIClass_QComboBox.cpp   （setupComboBoxStyle）
 * - CarbonUIClass_QStatusBar.cpp  （setupQStatusBarStyle）
 * - CarbonUIClass_QTreeWidget.cpp （applyTreeWidgetStyle / applyTreeViewStyle）
 *
 * 实现层面的通用约定：
 * - 每个助手开头都有 `if (!w) return;` 的判空，因此调用方可以传入可能缺失的控件；
 * - 样式字符串是内联字面量，没有主题系统，改配色就是改这些字面量；
 * - 悬停/点击这类交互不能用 QSS 表达时，改为在控件上安装本地事件过滤器，
 *   过滤器以控件为父对象，随控件一起销毁。
 *
 * 线程约定：全部函数都直接操作 QWidget，只能在 GUI 主线程调用。
 */

#include "CarbonUIClass.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

/**
 * @brief 注册子库内嵌资源（下拉箭头、树分支箭头等图片）。
 *
 * @details
 * 静态库里的 qrc 目标只有在注册符号被引用时才会被链接器从 .lib 中取出；
 * 没有这层显式引用时，`QComboBox::down-arrow` 的 url(:/img/...) 会在运行时
 * 解析为空图片（箭头不显示），而且不会报错。这里用构造期函数引用
 * qInitResources_carbonui_rcc，保证库一被链接资源就注册（幂等）。
 *
 * @note 必须放在文件级 static 函数里（不能放进匿名命名空间）：Q_INIT_RESOURCE
 *       展开的 extern 声明会被匿名命名空间改写成内部链接符号，导致链接失败。
 */
static void carbonuiRegisterResources()
{
    Q_INIT_RESOURCE(carbonui_rcc);
}

Q_CONSTRUCTOR_FUNCTION(carbonuiRegisterResources)


void CarbonUIClass::applyTooltipStyle(void)
{
    if (qApp)
    {
        QString appStyle = qApp->styleSheet();
        QString ttStyle = "QToolTip { background-color: #0f0f0f; color: #029de8; border: 2px solid #0f0f0f;  }";
        // always append tooltip style so the rounded corners are enforced even if a QToolTip rule exists
        qApp->setStyleSheet(appStyle + "\n" + ttStyle);
    }
}


void CarbonUIClass::setWidgetLabelsStyleRecursively(QWidget* w)
{
	if (!w) return;
	const QObjectList& children = w->children();
	for (QObject* o : children)
	{
		if (!o) continue;
		QLabel* lab = qobject_cast<QLabel*>(o);
		if (lab)
		{
			lab->setStyleSheet("color:  #676767;");
			continue;
		}
		QWidget* childWidget = qobject_cast<QWidget*>(o);
		if (childWidget)
		{
			setWidgetLabelsStyleRecursively(childWidget);
		}
	}
}
