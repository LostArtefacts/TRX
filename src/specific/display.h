#ifndef T1M_SPECIFIC_DISPLAY_H
#define T1M_SPECIFIC_DISPLAY_H

#include <stdint.h>

void SetupScreenSize();
void TempVideoAdjust(int32_t hi_res, double sizer);
void TempVideoRemove();
void S_NoFade();
void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);
void S_CopyBufferToScreen();

void T1MInjectSpecificDisplay();

#endif
