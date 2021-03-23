#ifndef T1M_SPECIFIC_INPUT_H
#define T1M_SPECIFIC_INPUT_H

#include "global/types.h"

#include <stdint.h>

// clang-format off
#define WinInReadJoystick       ((void          (*)())0x00437B00)
// clang-format on

extern int16_t Layout[2][KEY_NUMBER_OF];
extern int32_t Conflict[KEY_NUMBER_OF];
extern int32_t OldInputDB;

void OnKeyPress(int high, int key, int held);
void InputInit();
int32_t Key_(int32_t key);
int16_t KeyGet();
void KeyClearBuffer();
void S_UpdateInput();
int32_t GetDebouncedInput(int32_t input);

void T1MInjectSpecificInput();

#endif
