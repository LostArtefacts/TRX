#pragma once

#include <trx/core/math/types.h>

XYZ_16 Camera_PhotoMode_GetRot(void);

void Camera_PhotoMode_Enter(void);
void Camera_PhotoMode_Exit(void);
void Camera_PhotoMode_Update(void);
void Camera_PhotoMode_UpdateFOV(void);

void Camera_PhotoMode_Pause(void);
void Camera_PhotoMode_Resume(void);
