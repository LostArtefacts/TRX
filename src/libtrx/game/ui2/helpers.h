#pragma once

#include "game/ui2/common.h"

// Repetitive widget ops strategies
void UI2_MeasureWrapper(UI2_NODE *node);
void UI2_LayoutBasic(UI2_NODE *node, float x, float y, float w, float h);
void UI2_LayoutWrapper(UI2_NODE *node, float x, float y, float w, float h);
void UI2_DrawWrapper(const UI2_NODE *node);
