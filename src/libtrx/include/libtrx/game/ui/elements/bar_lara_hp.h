#pragma once

#include "../../../config/types.h"
#include "../common.h"

// state functions
void UI_LaraHealthBar_Control(void);
void UI_LaraHealthBar_SetTimer(int16_t timer);

// draw functions
bool UI_LaraHealthBar(bool blink_state, BAR_SHOW_MODE show_mode);
