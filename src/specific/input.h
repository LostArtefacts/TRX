#ifndef T1M_SPECIFIC_INPUT_H
#define T1M_SPECIFIC_INPUT_H

// clang-format off
#define Key_                    ((int          (*)(int number))0x0041E3E0)
#define WinInReadJoystick       ((void         (*)())0x00437B00)
// clang-format on

void S_UpdateInput();

void T1MInjectSpecificInput();

#endif
