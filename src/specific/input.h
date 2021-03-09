#ifndef T1M_SPECIFIC_INPUT_H
#define T1M_SPECIFIC_INPUT_H

// clang-format off
#define Key_                    ((int32_t       (*)(int32_t number))0x0041E3E0)
#define WinInReadJoystick       ((void          (*)())0x00437B00)
#define KeyClearBuffer          ((void          (*)())0x00437BD0)
// clang-format on

int16_t KeyGet();
void S_UpdateInput();

void T1MInjectSpecificInput();

#endif
