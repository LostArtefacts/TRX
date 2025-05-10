#pragma once

#include "../pathing/types.h"
#include "../rooms/types.h"

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
