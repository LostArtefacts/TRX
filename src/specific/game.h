#ifndef TOMB1MAIN_SPECIFIC_GAME_H
#define TOMB1MAIN_SPECIFIC_GAME_H

#include <stdint.h>

// clang-format off
#define S_SaveGame              ((void          __cdecl(*)())0x0041DB70)
#define GameLoop                ((int32_t       __cdecl(*)(int32_t demo_mode))0x0041D2C0)
// clang-format on

int __cdecl LevelIsValid(int16_t level_number);
void __cdecl SeedRandomControl(int32_t seed);
void __cdecl SeedRandomDraw(int32_t seed);
int32_t __cdecl GetRandomControl();
int32_t __cdecl GetRandomDraw();
void __cdecl LevelStats(int level_id);

void Tomb1MInjectSpecificGame();

#endif
