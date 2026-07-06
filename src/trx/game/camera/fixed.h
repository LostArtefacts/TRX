#pragma once

#include <trx/game/camera/types.h>

void Camera_InitialiseFixedObjects(int32_t num_objects);
int32_t Camera_GetFixedObjectCount(void);
int32_t Camera_GetDynamicFixedObjectIdx(void);
void Camera_UpdateDynamicFixedObject(XYZ_32 pos, int16_t room_num);
OBJECT_VECTOR *Camera_GetFixedObject(int32_t object_idx);
bool Camera_IsLocked(int32_t camera_num);

void Camera_InitialiseFlybys(int32_t num_cameras);
int32_t Camera_GetFlybyCount(void);
FLYBY_CAMERA *Camera_GetFlybyCamera(int32_t camera_idx);

void Camera_SetupSequences(void);
int32_t Camera_GetSequenceCount(void);
FLYBY_SEQUENCE *Camera_GetSequence(int32_t sequence_idx);
