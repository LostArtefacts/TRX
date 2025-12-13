#pragma once

#include <trx/game/types.h>

void Camera_InitialiseFixedObjects(int32_t num_objects);
int32_t Camera_GetFixedObjectCount(void);
OBJECT_VECTOR *Camera_GetFixedObject(int32_t object_idx);
bool Camera_IsLocked(int32_t camera_num);
