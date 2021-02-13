#ifndef TR1MAIN_GAME_GAME_H
#define TR1MAIN_GAME_GAME_H

#include <stdint.h>

int __cdecl LevelIsValid(int16_t level_number);
void __cdecl SeedRandomControl(int32_t seed);
void __cdecl SeedRandomDraw(int32_t seed);
int32_t __cdecl GetRandomControl();
int32_t __cdecl GetRandomDraw();
void __cdecl LevelStats(int level_id);

void TR1MInjectGame();

#endif
