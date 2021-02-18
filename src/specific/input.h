#ifndef TOMB1MAIN_SPECIFIC_INPUT_H
#define TOMB1MAIN_SPECIFIC_INPUT_H

// clang-format off
#define Key_                    ((int           __cdecl(*)(int number))0x0041E3E0)
#define WinInReadJoystick       ((void          __cdecl(*)())0x00437B00)
// clang-format on

void __cdecl S_UpdateInput();

void Tomb1MInjectSpecificInput();

#endif
