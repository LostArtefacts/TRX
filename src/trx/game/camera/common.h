#pragma once

#include <trx/config/types.h>
#include <trx/game/camera/types.h>
#include <trx/game/rooms/types.h>

void Camera_Update(void);
void Camera_UpdateMicPosition(void);
void Camera_EnsureEnvironment(void);
void Camera_MoveManual(void);
void Camera_Apply(void);
bool Camera_IsChunky(void);
void Camera_SetChunky(bool is_chunky);
void Camera_Initialise(void);
void Camera_ResetPosition(void);
void Camera_Reset(void);
void Camera_ClampInterpResult(void);
void Camera_RefreshFromTrigger(const TRIGGER *trigger);

void Camera_RegisterStrategy(CAMERA_MODE mode, CAMERA_STRATEGY strategy);

#define REGISTER_CAMERA(mode, strategy)                                        \
    __attribute__((constructor)) static void M_RegisterCamera##mode(void)      \
    {                                                                          \
        Camera_RegisterStrategy(mode, strategy);                               \
    }
