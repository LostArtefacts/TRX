#ifndef T1M_GAME_CAMERA_H
#define T1M_GAME_CAMERA_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define CalculateCamera         ((void         (*)())0x00410B40)
// clang-format on

void InitialiseCamera();
void MoveCamera(GAME_VECTOR* ideal, int32_t speed);
void ClipCamera(
    int32_t* x, int32_t* y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom);
void ShiftCamera(
    int32_t* x, int32_t* y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom);
int32_t BadPosition(int32_t x, int32_t y, int32_t z, int16_t room_num);
void SmartShift(
    GAME_VECTOR* ideal,
    void (*shift)(
        int32_t* x, int32_t* y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom));
void ChaseCamera(ITEM_INFO* item);
int32_t ShiftClamp(GAME_VECTOR* pos, int32_t clamp);

void T1MInjectGameCamera();

#endif
