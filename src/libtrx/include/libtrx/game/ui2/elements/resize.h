#pragma once

#include "../common.h"

// Resize child widget to a specified size in pixels.
// A negative size means to use the child's size.
// A zero value means to hide the child, but participate in the layout pass.

void UI2_BeginResize(float x, float y);
void UI2_EndResize(void);
