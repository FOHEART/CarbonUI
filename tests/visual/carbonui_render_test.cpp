/**
 * @file  carbonui_render_test.cpp
 * @brief 渲染证据用例：把样式化后的控件真正画出来，对像素颜色做断言。
 * @author CarbonUI
 * @date  2026-10-30
 * @version 1.0.0
 * @ingroup carbonui_tests
 *
 * @details
 * `tests/unit` 里的套件断言的是"配置"（样式表文本、几何、子控件），只能证明样式
 * 被写进去了；本套件走完整绘制管线（QWidget::grab() → QPainter → 光栅引擎），
 * 断言**画出来的像素**，因此能发现"QSS 写对了但根本看不见"这类缺陷
 * （选择器没命中、尺寸为 0、子控件被裁掉、圆角没生效……）。
 *
 * 取景方法：每个用例都渲染一个**父容器**，容器底色是醒目品红 \#ff00ff。
 * 控件圆角之外是透明的，只有父容器底色才能证明"这个角被切掉了"；
 * 单独 grab 一个有背景色的控件，四角也是自己的背景色，看不出圆角。
 *
 * 文字断言依赖平台字体库：`QT_QPA_PLATFORM=offscreen` 在 Windows 上字体库为空，
 * 此时用例会用 `GTEST_SKIP()` 跳过文字部分，填充/圆角断言仍然照跑。
 *
 * 采样陷阱：断言"填充色"时必须避开控件正中——标题就画在那里，字形连同抗锯齿边缘
 * 会覆盖中心像素，而不同机器（字体度量、提示方式、DPI）会让同一坐标恰好落在
 * "字母"或"字母间隙"上，造成只在 CI 上失败的假阴性。填充采样一律取内边距。
 *
 * 每次渲染都会把 PNG 存到可执行文件同级的 `ui-artifacts/`，可以直接打开肉眼复核。
 */

#include <gtest/gtest.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "CarbonUIClass.h"
#include "groupboxcollapser.h"
#include "qt_test_support.h"

namespace
{
/// 项目强调色：按钮底色与选中态单选按钮都用它。
const QString kAccent = QStringLiteral("#009ee8");
/// 未勾选复选框指示灯的灰色描边。
const QString kIndicatorGrey = QStringLiteral("#636363");
/// 数字输入框的深色填充。
const QString kSurface = QStringLiteral("#2f2f2f");
/// 未选中单选按钮的面板色。
const QString kPanel = QStringLiteral("#272727");
/// 父容器的识别色，只用于证明"圆角外透出了父容器"。
const QString kBackdrop = QStringLiteral("#ff00ff");

/**
 * @brief 建一个刷成 \#ff00ff 的父容器，作为渲染用例的取景框。
 *
 * @param[in] width  容器宽度，单位像素，取值 [1, 4096]。
 * @param[in] height 容器高度，单位像素，取值 [1, 4096]。
 * @return 新建容器，父对象为空，由调用方 delete 释放。
 */
QWidget* makeBackdrop(int width, int height)
{
    auto* host = new QWidget();
    host->setObjectName(QStringLiteral("host"));
    host->setStyleSheet(QStringLiteral("QWidget#host { background-color: #ff00ff; }"));
    host->resize(width, height);
    return host;
}
} // namespace

TEST(RenderEvidence, ButtonRendersAccentFillWithRoundedCorners)
{
    QWidget* host = makeBackdrop(160, 72);
    auto* button = new QPushButton(QStringLiteral("OK"), host);
    button->setGeometry(20, 20, 120, 32);
    CarbonUIClass::applyButtonStyle(button);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("01-button-round-all.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    // 按钮矩形左上角（20,20）必在圆角之外，应透出品红底色。
    EXPECT_EQ(carbonui_test::pixelName(image, 20, 20), kBackdrop)
        << "border-radius must actually clip the corner";
    // 填充采样必须避开正中：标题就画在那里，字形（含抗锯齿边缘）会覆盖中心像素，
    // 不同机器上的字体度量/提示差异会让同一个坐标落在"字母"或"字母间隙"上。
    // 左右内边距是纯填充区域，与字体无关。
    EXPECT_EQ(carbonui_test::pixelName(image, 25, 36), kAccent)
        << "left padding must be filled with the accent colour";
    EXPECT_EQ(carbonui_test::pixelName(image, 115, 36), kAccent)
        << "right padding must be filled with the accent colour";
}

TEST(RenderEvidence, RectangleButtonKeepsSquareCorner)
{
    QWidget* host = makeBackdrop(160, 72);
    auto* button = new QPushButton(QStringLiteral("OK"), host);
    button->setGeometry(20, 20, 120, 32);
    CarbonUIClass::applyButtonStyle(button, CarbonUIClass::Shape_Rectangle);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("02-button-rectangle.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    // 直角：同一点必须是强调色，与上一个用例形成对照。
    EXPECT_EQ(carbonui_test::pixelName(image, 20, 20), kAccent);
    // 填充断言同样避开正中标题，只取左侧内边距。
    EXPECT_EQ(carbonui_test::pixelName(image, 25, 36), kAccent)
        << "square button must stay filled right up to the corner";
}

TEST(RenderEvidence, ButtonTextIsReallyPainted)
{
    if (!carbonui_test::fontsAvailable())
        GTEST_SKIP() << "QPA platform provides no fonts; text pixels cannot be verified";

    QWidget* host = makeBackdrop(160, 72);
    auto* button = new QPushButton(QStringLiteral("Save"), host);
    button->setGeometry(20, 20, 120, 32);
    CarbonUIClass::applyButtonStyle(button);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("03-button-with-text.png"));

    bool foundWhite = false;
    for (int y = 20; y < 52 && !foundWhite; ++y)
        for (int x = 20; x < 140; ++x)
            if (image.pixelColor(x, y).name(QColor::HexRgb) == QStringLiteral("#ffffff"))
            {
                foundWhite = true;
                break;
            }
    EXPECT_TRUE(foundWhite) << "styled button caption must be painted in #ffffff";
    EXPECT_GT(carbonui_test::distinctColourCount(image), 3)
        << "a button with a caption cannot be a flat single-colour rectangle";

    delete host;
}

TEST(RenderEvidence, CheckedRadioRendersAccentFillWithRoundedCorners)
{
    QWidget* host = makeBackdrop(140, 60);
    auto* radio = new QRadioButton(QStringLiteral("Mode A"), host);
    radio->setGeometry(20, 16, 100, 28);
    CarbonUIClass::applyRadioStyle(radio, CarbonUIClass::Radio_FourRound, 72);
    radio->setChecked(true);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("04-radio-checked.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    // 采样点选在左上圆角之内、文字之外的留白处：文字居中，压到字形上会读到抗锯齿中间色。
    EXPECT_EQ(carbonui_test::pixelName(image, 26, 20), kAccent);
    EXPECT_EQ(carbonui_test::pixelName(image, 20, 16), kBackdrop)
        << "checked radio must keep its 12px corner radius";
}

TEST(RenderEvidence, UncheckedRadioRendersDarkPanel)
{
    QWidget* host = makeBackdrop(140, 60);
    auto* radio = new QRadioButton(QStringLiteral("Mode A"), host);
    radio->setGeometry(20, 16, 100, 28);
    CarbonUIClass::applyRadioStyle(radio, CarbonUIClass::Radio_FourRound, 72);
    radio->setChecked(false);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("05-radio-unchecked.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    EXPECT_EQ(carbonui_test::pixelName(image, 26, 20), kPanel);
}

TEST(RenderEvidence, SegmentShapesRenderAsOneContinuousBar)
{
    // 左圆 + 中直 + 右圆拼成一条分段选择器：拼缝处必须实心，两端必须切角。
    // 注意 QSS 的 min-width 会把控件撑到"内容宽度 + padding"，因此先按
    // minimumSizeHint() 取实际宽度，再首尾相接排布，避免相邻两格重叠。
    auto* left = new QRadioButton(QStringLiteral("L"));
    auto* middle = new QRadioButton(QStringLiteral("M"));
    auto* right = new QRadioButton(QStringLiteral("R"));
    CarbonUIClass::applyRadioStyle(left, CarbonUIClass::Radio_LeftRound, 60);
    CarbonUIClass::applyRadioStyle(middle, CarbonUIClass::Radio_Rectangle, 60);
    CarbonUIClass::applyRadioStyle(right, CarbonUIClass::Radio_RightRound, 60);
    // 同一个父控件里的 QRadioButton 默认自动互斥，会互相取消勾选；
    // 分段选择器本来就是"每段各自独立"，因此必须关掉互斥。
    left->setAutoExclusive(false);
    middle->setAutoExclusive(false);
    right->setAutoExclusive(false);

    const int segmentWidth = left->minimumSizeHint().width();
    const int segmentHeight = qMax(28, left->minimumSizeHint().height());
    ASSERT_GT(segmentWidth, 0);

    const int originX = 20;
    const int originY = 16;
    QWidget* host = makeBackdrop(originX * 2 + segmentWidth * 3, originY * 2 + segmentHeight);
    left->setParent(host);
    middle->setParent(host);
    right->setParent(host);
    left->setGeometry(originX, originY, segmentWidth, segmentHeight);
    middle->setGeometry(originX + segmentWidth, originY, segmentWidth, segmentHeight);
    right->setGeometry(originX + segmentWidth * 2, originY, segmentWidth, segmentHeight);
    left->setChecked(true);
    middle->setChecked(true);
    right->setChecked(true);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("06-radio-segments.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    const int seamOne = originX + segmentWidth;
    const int seamTwo = originX + segmentWidth * 2;
    const int midY = originY + segmentHeight / 2;

    // 两端：左格左上角、右格右上角都切角 → 透出品红。
    EXPECT_EQ(carbonui_test::pixelName(image, originX, originY), kBackdrop)
        << "left segment must keep its left rounding";
    EXPECT_EQ(carbonui_test::pixelName(image, seamTwo + segmentWidth - 1, originY), kBackdrop)
        << "right segment must keep its right rounding";
    // 拼缝：左格右边缘与中格左边缘都必须实心，否则中间会有洞。
    EXPECT_EQ(carbonui_test::pixelName(image, seamOne - 1, midY), kAccent);
    EXPECT_EQ(carbonui_test::pixelName(image, seamOne, midY), kAccent);
    EXPECT_EQ(carbonui_test::pixelName(image, seamTwo - 1, midY), kAccent);
    EXPECT_EQ(carbonui_test::pixelName(image, seamTwo, midY), kAccent);
}

TEST(RenderEvidence, CheckboxIndicatorColourFollowsCheckedState)
{
    QWidget* host = makeBackdrop(48, 48);
    auto* checkBox = new QCheckBox(host);
    checkBox->setGeometry(16, 16, 16, 16);
    CarbonUIClass::applyCheckStyle(checkBox);

    const QImage uncheckedImage = carbonui_test::renderToArtifact(host, QStringLiteral("07-checkbox-unchecked.png"));
    checkBox->setChecked(true);
    const QImage checkedImage = carbonui_test::renderToArtifact(host, QStringLiteral("08-checkbox-checked.png"));
    ASSERT_FALSE(uncheckedImage.isNull());
    ASSERT_FALSE(checkedImage.isNull());
    delete host;

    // 8x8 指示灯带 4px 描边，正中像素就是描边色。
    EXPECT_EQ(carbonui_test::pixelName(uncheckedImage, 24, 24), kIndicatorGrey);
    EXPECT_EQ(carbonui_test::pixelName(checkedImage, 24, 24), kAccent);
}

TEST(RenderEvidence, LineEditRendersDarkRoundedFieldWithBorder)
{
    QWidget* host = makeBackdrop(220, 60);
    auto* edit = new QLineEdit(host);
    edit->setGeometry(20, 16, 180, 28);
    CarbonUIClass::applyLineEditStyle_numberInput(edit);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("09-line-edit.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    // 中心是 #2f2f2f 填充，左边缘是 2px #5f5f5f 描边，左上角因 12px 圆角透出底色。
    EXPECT_EQ(carbonui_test::pixelName(image, 110, 30), kSurface);
    EXPECT_EQ(carbonui_test::pixelName(image, 20, 30), QStringLiteral("#5f5f5f"));
    EXPECT_EQ(carbonui_test::pixelName(image, 20, 16), kBackdrop);
}

TEST(RenderEvidence, TabWidgetRendersPaneAndSelectedTab)
{
    QWidget* host = makeBackdrop(260, 140);
    auto* tabs = new QTabWidget(host);
    tabs->setGeometry(10, 10, 240, 120);
    tabs->addTab(new QLabel(QStringLiteral("page one")), QStringLiteral("One"));
    tabs->addTab(new QLabel(QStringLiteral("page two")), QStringLiteral("Two"));
    CarbonUIClass::setupTabWidgetStyle(tabs);

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("10-tab-widget.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    // 面板填充 #3b3b3b；选中页签 #2e2e2e 比未选中的 #272727 更亮。
    EXPECT_EQ(carbonui_test::pixelName(image, 130, 100), QStringLiteral("#3b3b3b"));
    EXPECT_EQ(carbonui_test::pixelName(image, 30, 18), QStringLiteral("#2e2e2e"));
    EXPECT_EQ(carbonui_test::pixelName(image, 120, 18), QStringLiteral("#272727"));
}

TEST(RenderEvidence, TreeWidgetRendersHeaderSelectionBandAndBorder)
{
    QWidget* host = makeBackdrop(260, 150);
    auto* tree = new QTreeWidget(host);
    tree->setGeometry(10, 10, 240, 120);
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList() << QStringLiteral("Name") << QStringLiteral("Value"));
    for (int i = 0; i < 3; ++i)
    {
        auto* item = new QTreeWidgetItem(tree);
        item->setText(0, QStringLiteral("row %1").arg(i));
        item->setText(1, QStringLiteral("v%1").arg(i));
    }
    CarbonUIClass::applyTreeWidgetStyle(tree);
    tree->setCurrentItem(tree->topLevelItem(1));

    const QImage image = carbonui_test::renderToArtifact(host, QStringLiteral("11-tree-widget.png"));
    ASSERT_FALSE(image.isNull());
    delete host;

    // 表头 #3b3b3b、外框 2px #5f5f5f、选中行 #0078d7。
    EXPECT_EQ(carbonui_test::pixelName(image, 130, 18), QStringLiteral("#3b3b3b"));
    EXPECT_EQ(carbonui_test::pixelName(image, 10, 70), QStringLiteral("#5f5f5f"));

    bool foundSelection = false;
    for (int y = 25; y < 130 && !foundSelection; ++y)
        for (int x = 30; x < 240; ++x)
            if (image.pixelColor(x, y).name(QColor::HexRgb) == QStringLiteral("#0078d7"))
            {
                foundSelection = true;
                break;
            }
    EXPECT_TRUE(foundSelection) << "selected row must render with the #0078d7 band";
}

TEST(RenderEvidence, ExpandedGroupBoxPaintsBodyCollapsedOneDoesNot)
{
    auto buildBox = [](bool expanded) {
        auto* box = new QGroupBox(QStringLiteral("Advanced"));
        auto* layout = new QVBoxLayout(box);
        layout->addWidget(new QLabel(QStringLiteral("body text")));
        layout->addWidget(new QPushButton(QStringLiteral("Action")));
        QGroupBoxCollapser::installIntoGroupBox(box, nullptr, expanded);
        box->resize(240, box->sizeHint().height());
        return box;
    };

    QWidget* expandedHost = makeBackdrop(260, 160);
    QGroupBox* expandedBox = buildBox(true);
    expandedBox->setParent(expandedHost);
    expandedBox->setGeometry(10, 10, 240, expandedBox->sizeHint().height());
    const QImage expandedImage = carbonui_test::renderToArtifact(expandedHost, QStringLiteral("12-groupbox-expanded.png"));
    const int expandedHeight = expandedBox->height();

    QWidget* collapsedHost = makeBackdrop(260, 160);
    QGroupBox* collapsedBox = buildBox(false);
    collapsedBox->setParent(collapsedHost);
    collapsedBox->setGeometry(10, 10, 240, collapsedBox->sizeHint().height());
    const QImage collapsedImage = carbonui_test::renderToArtifact(collapsedHost, QStringLiteral("13-groupbox-collapsed.png"));
    const int collapsedHeight = collapsedBox->height();

    ASSERT_FALSE(expandedImage.isNull());
    ASSERT_FALSE(collapsedImage.isNull());

    // 折叠后内容区被真正隐藏：面板高度必须小于展开态。
    EXPECT_LT(collapsedHeight, expandedHeight)
        << "collapsed panel must be shorter, proving the body widget is hidden";
    // 展开态在内容区范围内有控件像素，折叠态只剩标题行。
    EXPECT_LT(carbonui_test::distinctColourCount(collapsedImage),
              carbonui_test::distinctColourCount(expandedImage))
        << "expanded panel must contain the body widgets' pixels";

    delete expandedHost;
    delete collapsedHost;
}
