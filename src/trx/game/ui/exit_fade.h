#pragma once

// The fade to black as the game exits. It draws in the exit layer, over every
// other layer of the interface, in every loop that draws an interface scene.

// Starts the fade, once. A second call leaves it running.
void UI_BeginExitFade(void);

// Reports whether the fade still has frames to draw.
bool UI_IsExitFading(void);

// Schedules the fade in the exit layer. UI_EndScene calls this.
void UI_DrawExitFade(void);
