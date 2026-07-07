#pragma once

#include <stdint.h>

bool Camera_FlybyMode_Activate(int32_t sequence, bool one_shot);
void Camera_FlybyMode_Deactivate(void);
bool Camera_FlybyMode_IsActive(void);
bool Camera_Flybymode_Cancel(void);

void Camera_FlybyMode_RequestLook(void);
void Camera_FlybyMode_Update(void);
