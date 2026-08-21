#pragma once

#include <trx/game/ui/common.h>

// Used to align a top-level widget to the screen center or to the screen edges.
// Uses ratio inputs. A ratio from 0 to 1 keeps the widget inside the space it
// is given, and pins it to the top left corner once it grows past that space.
// A ratio outside that range places the widget beyond the edge, which a widget
// that has to sit in the padding around its box relies on.

void UI_BeginAnchor(const float x, const float y);
void UI_EndAnchor(void);
