#pragma once

#include <trx/game/ui/common.h>

// A widget that resizes the children to the canvas size
// and places it at a specific proportional spot.

void UI_BeginModal(float x, float y);

// A modal that keeps the given height clear at the top and the bottom of the
// canvas, so that what it places lands within what is left rather than over the
// text drawn at the screen edges.
void UI_BeginModalEx(float x, float y, float inset_v);

void UI_EndModal(void);
