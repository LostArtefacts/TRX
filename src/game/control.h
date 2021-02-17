#ifndef TR1MAIN_GAME_CONTROL_H
#define TR1MAIN_GAME_CONTROL_H

#include <stdint.h>

// clang-format off
#define CheckCheatMode          ((void          __cdecl(*)())0x00438920)
#define GetFloor                ((FLOOR_INFO*   __cdecl(*)(int32_t x, int32_t y, int32_t z, int16_t* room_number))0x00413A80)
#define GetCeiling              ((int16_t       __cdecl(*)(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z))0x00414880)
#define GetHeight               ((int16_t       __cdecl(*)(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z))0x00413D60)
#define TestTriggers            ((void          __cdecl(*)(int16_t* data, int heavy))0x00414080)
// clang-format on

int32_t __cdecl ControlPhase(int32_t nframes, int demo_mode);

void TR1MInjectControl();

#endif
