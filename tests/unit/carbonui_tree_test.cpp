/**
 * @file  carbonui_tree_test.cpp
 * @brief `applyTreeWidgetStyle()` 与 `applyTreeViewStyle()` 的行为用例。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 见 include/CarbonUIVersion.h（工程版本唯一来源）
 * @ingroup carbonui_tests
 *
 * @details
 * 两者的缩进语义相反（表格型 QTreeWidget 压平为 0，层级型 QTreeView 保留 14px），
 * 并且都会改动 header 字体与编辑触发方式——这些是主程序树控件可用性的前提，
 * 因此逐项断言。
 */

#include <gtest/gtest.h>

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QTreeWidget>

#include "CarbonUIQTreeView.h"
#include "CarbonUIQTreeWidget.h"
#include "qt_test_support.h"

TEST(TreeWidgetStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQTreeWidget::applyStyle(nullptr));
}

TEST(TreeWidgetStyle, FlattensIndentationAndStylesHeader)
{
    QTreeWidget tree;

    CarbonUIQTreeWidget::applyStyle(&tree);

    EXPECT_EQ(tree.indentation(), 0);
    EXPECT_FALSE(tree.rootIsDecorated());
    EXPECT_EQ(tree.font().pointSize(), 13);
    EXPECT_FALSE(tree.font().bold());

    QHeaderView* header = tree.header();
    ASSERT_NE(header, nullptr);
    EXPECT_EQ(header->defaultAlignment(), Qt::AlignLeft | Qt::AlignVCenter);
    EXPECT_EQ(header->minimumHeight(), 24);
    EXPECT_TRUE(header->font().bold()) << "the header keeps the bold variant of the 13pt font";
    EXPECT_QSS_CONTAINS(tree.styleSheet(), "QTreeWidget { border: 2px solid #5f5f5f;");
    EXPECT_QSS_CONTAINS(tree.styleSheet(), "QTreeWidget::item:selected");
}

TEST(TreeViewStyle, NullWidgetIsIgnored)
{
    EXPECT_NO_FATAL_FAILURE(CarbonUIQTreeView::applyStyle(nullptr));
}

TEST(TreeViewStyle, KeepsHierarchyAndDisablesEditing)
{
    QTreeView view;
    view.setRootIsDecorated(false);
    view.setIndentation(0);

    CarbonUIQTreeView::applyStyle(&view);

    EXPECT_EQ(view.indentation(), 14);
    EXPECT_TRUE(view.rootIsDecorated());
    EXPECT_TRUE(view.itemsExpandable());
    EXPECT_TRUE(view.expandsOnDoubleClick());
    EXPECT_EQ(view.editTriggers(), QAbstractItemView::NoEditTriggers);
    EXPECT_QSS_CONTAINS(view.styleSheet(), "arrow_right.png");
    EXPECT_QSS_CONTAINS(view.styleSheet(), "QTreeView::item:selected");
}

TEST(TreeViewStyle, AcceptsTreeWidgetThroughBasePointer)
{
    QTreeWidget tree; // QTreeWidget derives from QTreeView

    CarbonUIQTreeView::applyStyle(&tree);

    EXPECT_EQ(tree.indentation(), 14);
    EXPECT_TRUE(tree.rootIsDecorated());
}
