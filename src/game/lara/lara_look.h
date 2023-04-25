#pragma once

// Lara lookaround routines.

#include "global/types.h"

#include <stdint.h>

void Lara_LookLeftRight(int16_t max_head_rot, int16_t head_turn);
void Lara_LookLeftRightSurf(int16_t max_head_rot, int16_t head_turn);
void Lara_LookUpDown(
    int16_t min_head_tilt, int16_t max_head_tilt, int16_t head_turn);
void Lara_LookUpDownSurf(
    int16_t min_head_tilt, int16_t max_head_tilt, int16_t head_turn);
void Lara_ResetLook(void);
