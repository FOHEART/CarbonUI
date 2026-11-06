/**
 * @file  groupboxcollapser_test.cpp
 * @brief `QGroupBoxCollapser::installIntoGroupBox()` 的折叠/展开行为用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 安装过程会重排 QGroupBox 的内部结构：原布局被搬进 `QGroupBoxBody`，
 * 标题搬进布局的 menuBar 槽里的可勾选 QToolButton，并清空 `QGroupBox::title()`。
 * 折叠/展开靠 180ms 的属性动画完成，因此用例只断言动画结束后的稳定状态，
 * 用事件泵等待而不是固定 `sleep`。
 */

#include <gtest/gtest.h>

#include <QtCore/QEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#include "groupboxcollapser.h"
#include "qt_test_support.h"

namespace
{
const char* const kBodyObjectName = "QGroupBoxBody";
const char* const kTitleObjectName = "QGroupBoxTitle";
} // namespace

/**
 * @brief QGroupBoxCollapser 用例的公共夹具：一个已填充内容的分组框。
 *
 * @details
 * 每个用例都从"一个标题为 Advanced、内含一个 label 的 QGroupBox"开始，
 * 这样断言只关注折叠结构本身，不受用例之间残留状态影响。
 */
class GroupBoxCollapserTest : public ::testing::Test
{
protected:
    /// 构造标题为 "Advanced"、含一个 QLabel 的分组框。
    void SetUp() override
    {
        m_box = new QGroupBox(QStringLiteral("Advanced"));
        m_layout = new QVBoxLayout(m_box);
        m_bodyLabel = new QLabel(QStringLiteral("body"), m_box);
        m_layout->addWidget(m_bodyLabel);
    }

    /// 释放分组框（控制器是其子 QObject，会一起销毁）。
    void TearDown() override
    {
        delete m_box; // the collapser is a child QObject of the box
        m_box = nullptr;
    }

    /// @return 标题行里的可勾选按钮；未安装时返回 NULL。
    QToolButton* toggleButton() const { return m_box->findChild<QToolButton*>(); }

    /// @return 内容区控件 "QGroupBoxBody"；未安装时返回 NULL。
    QWidget* body() const { return m_box->findChild<QWidget*>(QLatin1String(kBodyObjectName)); }

    /// @return 标题行控件 "QGroupBoxTitle"；未安装时返回 NULL。
    QWidget* titleRow() const { return m_box->findChild<QWidget*>(QLatin1String(kTitleObjectName)); }

    QGroupBox* m_box = nullptr;        ///< 被测分组框，TearDown 中销毁。
    QVBoxLayout* m_layout = nullptr;   ///< 安装前的原始布局，用于验证搬迁结果。
    QLabel* m_bodyLabel = nullptr;     ///< 原始内容控件，用于验证父对象变化。
};

TEST_F(GroupBoxCollapserTest, InstallReturnsControllerParentedToGroupBox)
{
    QGroupBoxCollapser* collapser = QGroupBoxCollapser::installIntoGroupBox(m_box);

    ASSERT_NE(collapser, nullptr);
    EXPECT_EQ(collapser->parent(), m_box) << "the controller must die with the group box";
}

TEST_F(GroupBoxCollapserTest, InstallMovesTitleIntoCheckableToggleButton)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);

    QToolButton* toggle = toggleButton();
    ASSERT_NE(toggle, nullptr);
    EXPECT_EQ(toggle->text(), QStringLiteral("Advanced"));
    EXPECT_TRUE(toggle->isCheckable());
    EXPECT_TRUE(toggle->isChecked());
    EXPECT_EQ(toggle->arrowType(), Qt::DownArrow);
    EXPECT_TRUE(m_box->title().isEmpty()) << "the native title is cleared after the move";
}

TEST_F(GroupBoxCollapserTest, InstallMovesOriginalLayoutIntoBodyWidget)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);

    QWidget* bodyWidget = body();
    ASSERT_NE(bodyWidget, nullptr);
    EXPECT_EQ(bodyWidget->layout(), m_layout) << "the caller's layout must survive the move";
    EXPECT_EQ(m_bodyLabel->parentWidget(), bodyWidget);
}

TEST_F(GroupBoxCollapserTest, InstallParksTitleRowInLayoutMenuBarSlot)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box);

    QWidget* titleWidget = titleRow();
    ASSERT_NE(titleWidget, nullptr);
    ASSERT_NE(m_box->layout(), nullptr);
    EXPECT_EQ(m_box->layout()->menuBar(), titleWidget);
    EXPECT_EQ(toggleButton()->parentWidget(), titleWidget)
        << "the toggle button must live inside the title row";
}

TEST_F(GroupBoxCollapserTest, InstallAppliesCollapserStyleAndCopiesItToChildren)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box);

    EXPECT_QSS_CONTAINS(m_box->styleSheet(), "border-bottom: 1px solid #000000");
    QWidget* titleWidget = titleRow();
    ASSERT_NE(titleWidget, nullptr);
    EXPECT_QSS_CONTAINS(titleWidget->styleSheet(), "border-bottom: 1px solid #000000");
}

TEST_F(GroupBoxCollapserTest, InstallWithCornerWidgetAddsItToTitleRow)
{
    auto* cornerWidget = new QPushButton(QStringLiteral("..."));

    QGroupBoxCollapser::installIntoGroupBox(m_box, cornerWidget, true);

    QWidget* titleWidget = titleRow();
    ASSERT_NE(titleWidget, nullptr);
    EXPECT_EQ(cornerWidget->parentWidget(), titleWidget);
}

TEST_F(GroupBoxCollapserTest, InstallCollapsedHidesBodyAndPointsRight)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, false);

    QWidget* bodyWidget = body();
    ASSERT_NE(bodyWidget, nullptr);
    EXPECT_TRUE(bodyWidget->isHidden());
    EXPECT_EQ(toggleButton()->arrowType(), Qt::RightArrow);
    EXPECT_FALSE(toggleButton()->isChecked());
}

TEST_F(GroupBoxCollapserTest, InstallExpandedKeepsBodyVisible)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);

    QWidget* bodyWidget = body();
    ASSERT_NE(bodyWidget, nullptr);
    EXPECT_FALSE(bodyWidget->isHidden());
    EXPECT_EQ(toggleButton()->arrowType(), Qt::DownArrow);
}

TEST_F(GroupBoxCollapserTest, ClickCollapsesBodyOnceAnimationFinishes)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);
    QWidget* bodyWidget = body();
    QToolButton* toggle = toggleButton();
    ASSERT_NE(bodyWidget, nullptr);
    ASSERT_NE(toggle, nullptr);

    toggle->click();

    EXPECT_FALSE(toggle->isChecked());
    EXPECT_EQ(toggle->arrowType(), Qt::RightArrow);
    EXPECT_TRUE(carbonui_test::spinUntil([bodyWidget]() { return bodyWidget->isHidden(); }))
        << "the body widget must be hidden after the collapse animation";

    toggle->click();

    EXPECT_TRUE(toggle->isChecked());
    EXPECT_EQ(toggle->arrowType(), Qt::DownArrow);
    EXPECT_TRUE(carbonui_test::spinUntil([bodyWidget]() {
        return !bodyWidget->isHidden() && bodyWidget->maximumHeight() == QWIDGETSIZE_MAX;
    })) << "the body widget must be visible again once expanded";
}

TEST_F(GroupBoxCollapserTest, AddWidgetAppendsToBodyLayout)
{
    QGroupBoxCollapser* collapser = QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);
    ASSERT_NE(collapser, nullptr);
    auto* extra = new QLabel(QStringLiteral("extra"));
    const int before = body()->layout()->count();

    collapser->addWidget(extra);

    EXPECT_EQ(body()->layout()->count(), before + 1);
    EXPECT_EQ(body()->layout()->indexOf(extra), before);
    EXPECT_EQ(extra->parentWidget(), body());
}

TEST_F(GroupBoxCollapserTest, AddWidgetIgnoresNull)
{
    QGroupBoxCollapser* collapser = QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);
    ASSERT_NE(collapser, nullptr);
    const int before = body()->layout()->count();

    EXPECT_NO_FATAL_FAILURE(collapser->addWidget(nullptr));

    EXPECT_EQ(body()->layout()->count(), before);
}

TEST_F(GroupBoxCollapserTest, TitleChangesAreResyncedFromGroupBox)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);
    QToolButton* toggle = toggleButton();
    ASSERT_NE(toggle, nullptr);

    m_box->setTitle(QStringLiteral("Renamed"));
    QEvent layoutRequest(QEvent::LayoutRequest);
    QApplication::sendEvent(m_box, &layoutRequest);

    EXPECT_EQ(toggle->text(), QStringLiteral("Renamed"));
    EXPECT_TRUE(m_box->title().isEmpty());
}

TEST_F(GroupBoxCollapserTest, EmptyTitleKeepsCurrentCaption)
{
    QGroupBoxCollapser::installIntoGroupBox(m_box, nullptr, true);
    QToolButton* toggle = toggleButton();
    ASSERT_NE(toggle, nullptr);

    m_box->setTitle(QString());
    QEvent layoutRequest(QEvent::LayoutRequest);
    QApplication::sendEvent(m_box, &layoutRequest);

    EXPECT_EQ(toggle->text(), QStringLiteral("Advanced"));
}
