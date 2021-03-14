#ifndef T1M_SPECIFIC_INPUT_H
#define T1M_SPECIFIC_INPUT_H

#include <stdint.h>

// clang-format off
#define WinInReadJoystick       ((void          (*)())0x00437B00)
// clang-format on

int32_t Key_(int32_t key);
int16_t KeyGet();
void KeyClearBuffer();
void S_UpdateInput();

void T1MInjectSpecificInput();

#endif
