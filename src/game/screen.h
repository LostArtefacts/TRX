#ifndef T1M_GAME_SCREEN_H
#define T1M_GAME_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

void SetupScreenSize();
bool SetGameScreenSizeIdx(int32_t idx);
bool SetPrevGameScreenSize();
bool SetNextGameScreenSize();
int32_t GetGameScreenSizeIdx();
int32_t GetGameScreenWidth();
int32_t GetGameScreenHeight();
int32_t GetScreenSizeIdx();
int32_t GetScreenWidth();
int32_t GetScreenHeight();

void TempVideoAdjust(int32_t hi_res);
void TempVideoRemove();

#endif
