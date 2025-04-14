#pragma once

#include "../common.h"

// An invisible border in pixel units around the child widget.

void UI2_BeginPad(float x, float y);
void UI2_BeginPadEx(float l, float r, float t, float d);
void UI2_EndPad(void);
