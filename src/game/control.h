#ifndef TR1MAIN_GAME_CONTROL_H
#define TR1MAIN_GAME_CONTROL_H

#include <stdint.h>

// clang-format off
#define CheckCheatMode          ((void          __cdecl(*)())0x00438920)
// clang-format on

int32_t __cdecl ControlPhase(int32_t nframes, int demo_mode);

void TR1MInjectControl();

#endif
