#pragma once

#include "../common.h"

// Make the child widget invisible if the given flag is true.
// The children still take up their size.

void UI2_BeginHide(bool hide_children);
void UI2_EndHide(void);
