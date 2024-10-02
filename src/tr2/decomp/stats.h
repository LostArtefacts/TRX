#pragma once

#include <stdint.h>

void __cdecl ShowGymStatsText(const char *time_str, int32_t type);
void __cdecl ShowStatsText(const char *time_str, int32_t type);
void __cdecl ShowEndStatsText(void);

int32_t __cdecl LevelStats(int32_t level_num);
int32_t __cdecl GameStats(int32_t level_num);
int32_t __cdecl AddAssaultTime(uint32_t time);
