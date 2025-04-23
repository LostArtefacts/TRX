#pragma once

#include "../common.h"

// A transformer to detach the children from the parent layout.
// x, y specify the anchor point - 0,0 mean top left corner, 1,1 mean bottom
// right corner.

void UI_BeginFixed(float x, float y);
void UI_EndFixed(void);
