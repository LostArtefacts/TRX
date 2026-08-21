#pragma once

#include <trx/game/ui/common.h>

// Places the widgets it holds within the area a dialog may occupy: the canvas
// less the screen margin, less the inset the overlay and the inventory ring
// keep clear at the top and the bottom. The spot is proportional to that area.
void UI_BeginModal(float x, float y);

// Places the widgets it holds over the whole canvas, edges included. The
// overlay, the console and the inventory ring use this, because they draw the
// screen edges and so state the area the dialogs get.
void UI_BeginScreenModal(float x, float y);

void UI_EndModal(void);
