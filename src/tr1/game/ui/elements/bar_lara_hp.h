#pragma once

#include <libtrx/config/types.h>
#include <libtrx/game/ui/common.h>

// state functions
void UI_LaraHealthBar_Control(void);
void UI_LaraHealthBar_SetTimer(int16_t timer);

// draw functions
bool UI_LaraHealthBar(bool blink_state, BAR_SHOW_MODE show_mode);
