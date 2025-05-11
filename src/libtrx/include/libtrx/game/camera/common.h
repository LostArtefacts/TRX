#pragma once

#include "../pathing/types.h"
#include "../rooms/types.h"

// TODO: make private
#define CAMERA_SHIFT_ARGS                                                      \
    int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,    \
        int32_t target_h, int32_t left, int32_t top, int32_t right,            \
        int32_t bottom

extern void Camera_Update(void);
void Camera_Apply(void);
void Camera_Move(const GAME_VECTOR *target, int32_t speed);
bool Camera_IsChunky(void);
void Camera_SetChunky(bool is_chunky);
void Camera_Initialise(void);
void Camera_ResetPosition(void);
void Camera_Reset(void);
void Camera_ClampInterpResult(void);
void Camera_RefreshFromTrigger(const TRIGGER *trigger);

const BOX_INFO *Camera_GetBox(const SECTOR *sector, int32_t x, int32_t z);
bool Camera_IsGoodPosition(int32_t x, int32_t y, int32_t z, int16_t room_num);
const SECTOR *Camera_GetSector(
    int32_t x, int32_t y, int32_t z, int16_t room_num);
int32_t Camera_ShiftClamp(GAME_VECTOR *pos, int32_t clamp);
void Camera_SmartShift(GAME_VECTOR *target, void (*shift)(CAMERA_SHIFT_ARGS));
void Camera_Clip(CAMERA_SHIFT_ARGS);
void Camera_Shift(CAMERA_SHIFT_ARGS);
void Camera_Chase(const ITEM *item);
void Camera_Combat(const ITEM *item);
void Camera_Fixed(void);
void Camera_Look(const ITEM *item);
