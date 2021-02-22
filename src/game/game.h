#ifndef T1M_GAME_GAME_H
#define T1M_GAME_GAME_H

#include <stdint.h>

// clang-format off
#define S_SaveGame              ((void         (*)())0x0041DB70)
#define GameLoop                ((int32_t      (*)(int32_t demo_mode))0x0041D2C0)
// clang-format on

int LevelIsValid(int16_t level_number);
void SeedRandomControl(int32_t seed);
void SeedRandomDraw(int32_t seed);
int32_t GetRandomControl();
int32_t GetRandomDraw();
void LevelStats(int level_id);

void T1MInjectSpecificGame();

#endif
