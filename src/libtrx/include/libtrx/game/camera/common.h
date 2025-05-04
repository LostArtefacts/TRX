#pragma once

#include "../rooms/types.h"

extern void Camera_Update(void);
extern void Camera_Apply(void);
bool Camera_IsChunky(void);
void Camera_SetChunky(bool is_chunky);
void Camera_Reset(void);
void Camera_ClampInterpResult(void);
void Camera_RefreshFromTrigger(const TRIGGER *trigger);
