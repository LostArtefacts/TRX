#pragma once

#include "../rooms/types.h"

void Camera_Update(void);
void Camera_UpdateMicPosition(void);
void Camera_MoveManual(void);
void Camera_Apply(void);
bool Camera_IsChunky(void);
void Camera_SetChunky(bool is_chunky);
void Camera_Initialise(void);
void Camera_ResetPosition(void);
void Camera_Reset(void);
void Camera_ClampInterpResult(void);
void Camera_RefreshFromTrigger(const TRIGGER *trigger);
