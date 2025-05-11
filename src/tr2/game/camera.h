#pragma once

#include "global/types.h"

#include <libtrx/game/camera.h>

void Camera_Clip(CAMERA_SHIFT_ARGS);
void Camera_Shift(CAMERA_SHIFT_ARGS);
void Camera_Chase(const ITEM *item);
void Camera_Combat(const ITEM *item);
void Camera_Look(const ITEM *item);
void Camera_Fixed(void);
void Camera_LoadCutsceneFrame(void);
void Camera_UpdateCutscene(void);
