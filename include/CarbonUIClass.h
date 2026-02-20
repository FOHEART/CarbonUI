/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CARBONUICLASS_H
#define CARBONUICLASS_H

#include <QtWidgets/QRadioButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QStatusBar>

#include "carbonui_global.h"

class CARBONUI_DLL_API CarbonUIClass
{
public:
    // radio shape options
    enum RadioShape { Radio_FourRound = 0, Radio_LeftRound, Radio_RightRound, Radio_Rectangle };

    static void applyTooltipStyle();
    // apply radio appearance to a single QRadioButton using the specified shape
    static void applyRadioStyle(QRadioButton* rb, RadioShape shape = Radio_FourRound, int minWidth = 28);

    static void applyLineEditStyle_numberInput(QLineEdit* le, int width = 0);
    static void applyCheckStyle(QCheckBox* cb);
    static void applyScrollAreaStyle(QScrollArea* area);

    // button shape options
    enum ButtonShape { Shape_RoundAll = 0, Shape_LeftRound, Shape_Rectangle, Shape_RightRound };

    // apply standard button appearance to a single QPushButton with optional shape
    static void applyButtonStyle(QPushButton* btn, ButtonShape shape = Shape_RoundAll);

    static void setupMenuBarStyle(QMenuBar* b);
    static void setupToolBarStyle(QToolBar* tb);
    static void setupTabWidgetStyle(QTabWidget* tw);
	static void setupComboBoxStyle(QComboBox* cb);
	static void setupQStatusBarStyle(QStatusBar* sb);

    static void applyTreeWidgetStyle(QTreeWidget* tree);
    static void setWidgetLabelsStyleRecursively(QWidget* w);


private:
    // backward-compatible wrappers
    static inline void applyRadioStyleLeftRound(QRadioButton* rb) { applyRadioStyle(rb, Radio_LeftRound); }
    static inline void applyRadioStyleRightRound(QRadioButton* rb) { applyRadioStyle(rb, Radio_RightRound); }
    static inline void applyRadioStyleRectangle(QRadioButton* rb) { applyRadioStyle(rb, Radio_Rectangle); }
    static inline void applyRadioStyleFourRoundCorner(QRadioButton* rb){ applyRadioStyle(rb, Radio_FourRound); }
};

#endif // CARBONUICLASS_H
