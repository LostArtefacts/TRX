#ifndef T1M_SPECIFIC_DISPLAY_H
#define T1M_SPECIFIC_DISPLAY_H

// clang-format off
#define TempVideoRemove         ((void          (*)())0x004167D0)
// clang-format on

void SetupScreenSize();
void TempVideoAdjust(int hi_res, double sizer);
void S_NoFade();
void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);

void T1MInjectSpecificDisplay();

#endif
