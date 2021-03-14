#ifndef T1M_SPECIFIC_DISPLAY_H
#define T1M_SPECIFIC_DISPLAY_H

#include <stdint.h>

// clang-format off
#define S_CopyBufferToScreen    ((void          (*)())0x00416A60)
// clang-format on

void SetupScreenSize();
void TempVideoAdjust(int32_t hi_res, double sizer);
void TempVideoRemove();
void S_NoFade();
void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);

void T1MInjectSpecificDisplay();

#endif
