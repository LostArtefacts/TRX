#pragma once

#include <stdint.h>

#define DIFFICULTY_SELECT_SHUTDOWN 0
#define DIFFICULTY_SELECT_INIT 1
#define DIFFICULTY_SELECT_HIDEALL 2
#define DIFFICULTY_SELECT_WAITING_INPUT 3
#define DIFFICULTY_SELECT_IN_BACKGROUND 4
#define DIFFICULTY_SELECT_CONFIRM 5

int16_t Difficulty_GetCurrentIndex(float damages_to_lara_multiplier);

void Difficulty_GetTextStat(char *str, float damages_to_lara_multiplier);

void Difficulty_GetTextStat_NoHeader(
    char *str, float damages_to_lara_multiplier);

void Difficulty_Select(int16_t difficulty_select_mode);
