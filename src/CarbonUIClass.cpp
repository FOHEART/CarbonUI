/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file  CarbonUIClass.cpp
 * @brief CarbonUIClass 中"通用控件"样式助手的实现。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui
 *
 * @details
 * 本文件实现除单选按钮以外的全部样式助手（单选按钮见
 * CarbonUIClass_QRadioButton.cpp）。公开接口的完整参数说明写在
 * CarbonUIClass.h，这里只补充实现层面的约定：
 * - 每个助手开头都有 `if (!w) return;` 的判空，因此调用方可以传入可能缺失的控件；
 * - 样式字符串是内联字面量，没有主题系统，改配色就是改这些字面量；
 * - 悬停/点击这类交互不能用 QSS 表达时，改为在控件上安装本地事件过滤器，
 *   过滤器以控件为父对象，随控件一起销毁。
 *
 * 线程约定：全部函数都直接操作 QWidget，只能在 GUI 主线程调用。
 */

#include "CarbonUIClass.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QDoubleValidator>
#include <QtCore/QLocale>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QHeaderView>

#include <QtGui/QIcon>
#include <QtWidgets/QApplication>

/**
 * @brief 聚焦时自动全选文本的事件过滤器。
 *
 * @details
 * 数字输入框的典型用法是"整段替换"：鼠标点进来、或键盘 Tab 进来时把原有数值
 * 全选，用户直接敲新值即可覆盖。因为 `QLineEdit` 自身没有这个开关，这里用
 * `QTimer::singleShot(0, ...)` 把全选推迟到 Qt 完成自己的焦点/鼠标处理后执行，
 * 否则会在鼠标按下时被随后的光标定位覆盖。
 *
 * 该过滤器以被监视的 `QLineEdit` 为父对象，随输入框一起销毁。
 */
class SelectAllOnFocusFilter : public QObject
{
public:
    /**
     * @brief 构造过滤器。
     * @param[in] parent 父对象，通常传被监视的 QLineEdit；可为 NULL。
     */
    SelectAllOnFocusFilter(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief 在获得焦点或用鼠标按下时把文本全选。
     *
     * @param[in] watched 事件接收者，期望是 QLineEdit；其它类型原样放行。
     * @param[in] event   事件对象，不可为 NULL。
     * @return 恒定返回 false，事件继续交给 QLineEdit 自身处理。
     */
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (auto le = qobject_cast<QLineEdit*>(watched))
        {
            if (event->type() == QEvent::FocusIn)
            {
                QTimer::singleShot(0, le, [le]() { le->selectAll(); });
            }
            else if (event->type() == QEvent::MouseButtonPress)
            {
                if (!le->hasFocus())
                {
                    QTimer::singleShot(0, le, [le]() { le->selectAll(); });
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

namespace
{
/**
 * @brief 复选框的自绘交互过滤器：悬停高亮 + 左键切换 + 指示灯配色。
 *
 * @details
 * QSS 无法表达"悬停时描边变白、勾选时描边变蓝且指示灯同步换色"这套效果，
 * 也无法让 0x0 的原生 indicator 之外再多画一个圆点，因此在复选框上安装
 * 本过滤器接管 Enter / Leave / MouseButtonRelease 三类事件。
 */
class HoverWatcher : public QObject
{
public:
    /**
     * @brief 构造过滤器并挂到复选框上。
     * @param[in] indicator 复选框内部的指示灯子控件，不可为 NULL，不取得所有权。
     * @param[in] cb        被监视的复选框，不可为 NULL，作为本对象的父对象。
     */
    HoverWatcher(QWidget* indicator, QCheckBox* cb) : QObject(cb), m_indicator(indicator), m_cb(cb) {}

    /**
     * @brief 处理悬停与左键释放。
     *
     * @details
     * - Enter：描边改为白色；
     * - Leave：恢复按勾选状态决定的描边色（蓝 / 灰）；
     * - MouseButtonRelease 且为左键：切换勾选状态并**吞掉**事件，避免
     *   QCheckBox 自身的按下/抬起逻辑再切换一次。
     *
     * @param[in] watched 事件接收者，本过滤器只处理复选框自身的事件。
     * @param[in] event   事件对象，不可为 NULL。
     * @return true 表示事件已被处理且不再向下传递（仅左键释放时）；
     *         其余情况返回基类结果 false。
     */
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Enter)
        {
            m_cb->setStyleSheet(
                "QCheckBox { background-color: transparent; border: 2px solid #ffffff; border-radius: 8px;} "
                "QCheckBox::indicator { width: 0px; height: 0px; } "
            );
        }
        else if (event->type() == QEvent::Leave)
        {
            if (m_cb->isChecked())
            {
                m_cb->setStyleSheet(
                    "QCheckBox { background-color: transparent; border: 2px solid #009ee8; border-radius: 8px;} "
                    "QCheckBox::indicator { width: 0px; height: 0px; } "
                );
                m_indicator->setStyleSheet("background-color: transparent; border: 4px solid #009ee8; border-radius: 4px;");
            }
            else
            {
                m_cb->setStyleSheet(
                    "QCheckBox { background-color: transparent; border: 2px solid #636363; border-radius: 8px;} "
                    "QCheckBox::indicator { width: 0px; height: 0px; } "
                );
                m_indicator->setStyleSheet("background-color: transparent; border: 4px solid #636363; border-radius: 4px;");
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton)
            {
                m_cb->setChecked(!m_cb->isChecked());
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }
private:
    QWidget* m_indicator;  ///< 被同步换色的指示灯子控件，不取得所有权。
    QCheckBox* m_cb;       ///< 被监视的复选框，同时是本对象的父对象。
};
} // namespace


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


void CarbonUIClass::applyLineEditStyle_numberInput(QLineEdit* le, int width)
{
    if (!le) return;

    le->setStyleSheet(
        "QLineEdit { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; border: 2px solid #5f5f5f; border-radius: 12px; padding: 2px 5px; } "
        "QLineEdit:hover { color: #dddddd; border-color: #777777; } ");

    le->setCursor(Qt::IBeamCursor);
    le->setContentsMargins(0, 0, 0, 0);
    le->setAlignment(Qt::AlignCenter);

    if (width > 0)
        le->setFixedWidth(width);

    auto* filter = new SelectAllOnFocusFilter(le);
    le->installEventFilter(filter);

    QObject::connect(le, &QLineEdit::returnPressed, le, [le]() { le->clearFocus(); });

    auto* validator = new QDoubleValidator(-1e12, 1e12, 6, le);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    le->setValidator(validator);
}

void CarbonUIClass::applyCheckStyle(QCheckBox* cb)
{
    if (!cb) return;

    cb->setStyleSheet(
        "QCheckBox { background-color: transparent; border: 2px solid #636363; border-radius: 8px;} "
        "QCheckBox::indicator { width: 0px; height: 0px; } "
        "QCheckBox:hover { border: 2px solid #ffffff; } "
        "QCheckBox:checked { border: 2px solid #009ee8; } "
    );

    cb->setContentsMargins(0, 0, 0, 0);
    cb->setFixedSize(16, 16);
    cb->setCursor(Qt::PointingHandCursor);
    cb->setAttribute(Qt::WA_Hover, true);
    cb->setMouseTracking(true);

    QString txt = cb->text();
    Q_UNUSED(txt);
    cb->setText("");
    auto* indicator = new QWidget(cb);
    indicator->setObjectName(cb->objectName() + QStringLiteral("_indicator"));
    int indicatorSize = 8;
    indicator->setFixedSize(indicatorSize, indicatorSize);
    indicator->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* lay = new QHBoxLayout(cb);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(indicator, 0, Qt::AlignCenter);

    auto setIndicatorStyle = [indicator](bool checked, int indicatorSize) {
        indicator->show();
        QString style = QString("background-color: transparent; border: %1px solid %2; border-radius: %3px;")
            .arg(indicatorSize / 2)
            .arg(checked ? QString("#009ee8") : QString("#636363"))
            .arg(indicatorSize / 2);
        indicator->setStyleSheet(style);
    };

    setIndicatorStyle(cb->isChecked(), indicatorSize);
    QObject::connect(cb, &QAbstractButton::toggled, cb, [indicator, indicatorSize](bool checked) {
        QString style = QString("background-color: transparent; border: %1px solid %2; border-radius: %3px;")
            .arg(indicatorSize / 2)
            .arg(checked ? QString("#009ee8") : QString("#636363"))
            .arg(indicatorSize / 2);
        indicator->setStyleSheet(style);
    });


    auto* watcher = new HoverWatcher(indicator, cb);
    cb->installEventFilter(watcher);
}

void CarbonUIClass::applyScrollAreaStyle(QScrollArea* area)
{
    if (!area) return;

    area->setStyleSheet(
        "QScrollArea { border: none; }"
        "QScrollArea QWidget { background-color: #3b3b3b; }"
        "QScrollArea QScrollBar:vertical { width: 4px; background: transparent; margin: 0px; }"
        "QScrollArea QScrollBar::handle:vertical { background: #cdcdcd; min-height: 20px; border-radius: 4px; }"
        "QScrollArea QScrollBar::add-line:vertical, QScrollArea QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollArea QScrollBar::add-page:vertical, QScrollArea QScrollBar::sub-page:vertical { background: none; }"
    );

    // ensure scrollbars have minimal width
    if (auto* v = area->verticalScrollBar())
    {
        v->setMinimumWidth(4);
    }
}

void CarbonUIClass::applyButtonStyle(QPushButton* btn, ButtonShape shape)
{
    if (!btn) return;

    // determine corner radii depending on shape
    int tl = 0, tr = 0, bl = 0, br = 0;
    switch (shape)
    {
        case Shape_RoundAll: tl = tr = bl = br = 10; break;
        case Shape_LeftRound: tl = bl = 10; tr = br = 0; break;
        case Shape_Rectangle: tl = tr = bl = br = 0; break;
        case Shape_RightRound: tr = br = 10; tl = bl = 0; break;
    }

    QString stylesheet = QString(
        "QPushButton { background-color: #009ee8; color: #ffffff; border: none; padding: 0px 2px; min-height: 28px; "
        "border-top-left-radius: %1px; border-bottom-left-radius: %2px; border-top-right-radius: %3px; border-bottom-right-radius: %4px; }")
        .arg(tl).arg(bl).arg(tr).arg(br);
    stylesheet += "QPushButton:hover { background-color: #00b0ff; }";
    stylesheet += "QPushButton:pressed { background-color: #007bb3; }";
    // 禁用态必须有独立规则，否则按钮被 setEnabled(false) 后仍渲染成可用态，
    // 用户分不清能不能点。灰底取自主题里既有的"非激活灰"，文字压暗但保持可读。
    stylesheet += "QPushButton:disabled { background-color: #636363; color: #cfcfcf; }";

    btn->setStyleSheet(stylesheet);
    btn->setCursor(Qt::PointingHandCursor);
}

void CarbonUIClass::setupMenuBarStyle(QMenuBar* b)
{
	if (!b)
		return;

	// central color for menu background/border so it can be changed easily
	QString menuBg = "#3b3b3b";

	b->setNativeMenuBar(false);
	QFont mbFont = b->font();
	mbFont.setPointSize(10);
	mbFont.setBold(true);
	b->setFont(mbFont);
	b->setFixedHeight(26);

	QString style = QString(
		"QMenuBar { background: %1; color: #cdcdcd; spacing: 0px; padding: 2px 0px; margin: 0px; }"
		"QMenuBar::item { background: transparent; padding: 2px 10px; margin: 0px; border-radius: 0px; }"
		"QMenuBar::item:hover { background: #2e2e2e;  color: #cdcdcd; }"
		"QMenuBar::item:selected { background: #2e2e2e; color: #cdcdcd; }"
		"QMenu { background: #2e2e2e; color: #d0d0d0; border: 1px solid %1; }"
		"QMenu::item:selected { background: %1; color: #cdcdcd; }"
	).arg(menuBg);

	b->setStyleSheet(style);
}

void CarbonUIClass::setupToolBarStyle(QToolBar* tb)
{
    if (!tb) return;

    // set toolbar icon size and add down icon
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setFixedHeight(32);
 tb->setIconSize(QSize(28, 28));

    int iconWidth = tb->iconSize().width();
    //int spacing = iconWidth / 4; // horizontal spacing = half icon width
    int spacing = 2;
    QString tbStyle = QString(
        "QToolBar { background: #3b3b3b;  border-top: 1px solid #212121; }"
        "QToolBar::handle { width: 0px; }"
        "QToolButton { border: none; border-radius: 10px; padding: 2px; }"
        "QToolButton:hover { border: 1px solid #5d5d5d; }"
    );

    tb->setStyleSheet(tbStyle);

    if (tb->layout())
    {
        tb->layout()->setSpacing(spacing);
    }

    tb->setContentsMargins(32, 0, spacing / 2, 0);

  
}

void CarbonUIClass::setupTabWidgetStyle(QTabWidget* tw)
{
    if (!tw) return;

    QString style =
        "QTabWidget::pane { background: #3b3b3b; border-radius: 0px; border: 1px solid #2b2b2b; }"
        "QTabBar { padding-right: 66px; }"
        "QTabBar::tab { background: #272727; color: #575757; padding: 8px 20px; margin-right: 0px; margin-top: 0px; border: none; }"
        "QTabBar::tab:selected { background: #2e2e2e; color: #cdcdcd; }"
        "QTabBar::scroller { background-color: transparent; }"
        "QTabBar::scroller QToolButton { background-color: #3b3b3b; color: #cdcdcd; border: none; padding: 0px; margin: 0px; min-width: 22px; min-height: 22px; border-radius: 0px; }";

    tw->setStyleSheet(style);
}

void CarbonUIClass::setupComboBoxStyle(QComboBox* cb)
{
	if (!cb) return;
    QString style = (
		"QComboBox { min-height: 24px; background-color: #2f2f2f; color: #cdcdcd; border: 2px solid #5f5f5f; border-radius: 12px; padding: 2px 34px 2px 8px; } "
		"QComboBox:hover { color: #dddddd; border-color: #777777; } "
		"QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; background-color: #2f2f2f; border-left: 1px solid #5f5f5f; border-top-right-radius: 10px; border-bottom-right-radius: 10px; } "
		"QComboBox::down-arrow { width: 24px; height: 24px; image: url(:/img/Resources/img/arrow_down.png); subcontrol-origin: padding; subcontrol-position: center ; } "
		"QComboBox::down-arrow:on { image: url(:/img/Resources/img/arrow_down.png); } "
		"QComboBox::icon { image: none; } "
		"QComboBox QAbstractItemView { color: white; background-color: #2f2f2f; selection-background-color: rgb(60,140,255); selection-color: white; }"
	);
    cb->setStyleSheet(style);
}

void CarbonUIClass::setupQStatusBarStyle(QStatusBar* sb)
{
    if (!sb)
        return;
   sb->setStyleSheet("background-color: #2e2e2e; color: #676767; border: none; outline: none;");
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

// helper to apply consistent styling to a QTreeWidget
 void CarbonUIClass::applyTreeWidgetStyle(QTreeWidget* tree)
{
	if (!tree)
		return;

	// style tree widget: outer border 2px solid #5f5f5f, header background and text, header bottom inner border 1px
	tree->setStyleSheet(
		"QTreeWidget { border: 2px solid #5f5f5f; background-color: #2f2f2f; color: #cdcdcd; }"
		"QHeaderView::section { background-color: #3b3b3b; color: #ffffff; border: 1px solid #5f5f5f; font-weight: bold; }"
		"QTreeWidget::item { border: 1px solid #5f5f5f; padding-left: 0px; min-height: 24px; }"
		"QTreeWidget::item:selected, QTreeView::item:selected { background-color: #0078d7; color: #ffffff; }"
	);

	// remove left indentation/spacing for first column and align header left
	tree->setIndentation(0);
	tree->setRootIsDecorated(false);

	QFont mbFont = tree->font();
	mbFont.setPointSize(13);
	mbFont.setBold(false);
	if (tree->header())
	{
		tree->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		tree->header()->setMinimumHeight(24);
		mbFont.setBold(true);
		tree->header()->setFont(mbFont);
	}

	mbFont.setBold(false);
	tree->setFont(mbFont);
}

// helper to apply consistent styling to a QTreeView (or subclasses)
void CarbonUIClass::applyTreeViewStyle(QTreeView* view)
{
	if (!view)
		return;

	// Use resource images for branch indicators
    view->setStyleSheet(R"(
            QTreeView { background-color: transparent; color: white; border: none; }
            QTreeView::item { color: white; border: none; }
            QTreeView::item:selected { background-color: #2e2e2e; color: white; border: none; }
            QTreeView::item:selected:active { background-color: #2e2e2e; color: white; }
            QTreeView::item:focus { outline: none; }
            QTreeView::branch { background: transparent; }
            /* Closed (right-pointing) arrow - white image resource */
            QTreeView::branch:closed:has-children {
                image: url(":/images/Resources/img/arrow_right.png");
                width: 24px; height: 24px; }
            /* Open (down-pointing) arrow - white image resource */
            QTreeView::branch:open:has-children {
                image: url(":/images/Resources/img/arrow_down.png");
                width: 24px; height: 24px; }
        )");

	// Ensure the view shows branch indicators and can expand
	view->setRootIsDecorated(true);
	view->setItemsExpandable(true);
	view->setExpandsOnDoubleClick(true);
	// Ensure there is space for the branch icon
    view->setIndentation(14);

	// Disable editing via the view
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
}
