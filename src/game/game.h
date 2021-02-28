#ifndef T1M_GAME_GAME_H
#define T1M_GAME_GAME_H

#include <stdint.h>

// clang-format off
#define S_SaveGame              ((void          (*)())0x0041DB70)
// clang-format on

int32_t GameLoop(int demo_mode);
int32_t LevelIsValid(int16_t level_num);
int32_t LevelCompleteSequence(int level_num);
void SeedRandomControl(int32_t seed);
void SeedRandomDraw(int32_t seed);
int32_t GetRandomControl();
int32_t GetRandomDraw();
void LevelStats(int32_t level_num);
int32_t S_LoadGame(void* data, int32_t size, int slot);
void GetSavedGamesList(REQUEST_INFO* req);

void T1MInjectGameGame();

#endif
