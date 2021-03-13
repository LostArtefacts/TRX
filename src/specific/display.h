#ifndef T1M_SPECIFIC_DISPLAY_H
#define T1M_SPECIFIC_DISPLAY_H

// clang-format off
#define TempVideoRemove         ((void          (*)())0x004167D0)
#define TempVideoAdjust         ((void          (*)(int32_t hires, double sizer))0x00416550)
#define S_NoFade                ((void          (*)())0x00416B10)
// clang-format on

void SetupScreenSize();
void S_FadeInInventory(int32_t fade);
void S_FadeOutInventory(int32_t fade);

void T1MInjectSpecificDisplay();

#endif
