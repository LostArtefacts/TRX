#pragma once

#include "../items/types.h"
#include "./types.h"

void Camera_InitialiseCineFrames(int32_t num_frames);
CINE_FRAME *Camera_GetCineFrame(int32_t frame_idx);
CINE_FRAME *Camera_GetCurrentCineFrame(void);
CINE_DATA *Camera_GetCineData(void);
void Camera_InvokeCinematic(
    const ITEM *item, int32_t frame_idx, int16_t extra_y_rot);

extern void Camera_LoadCutsceneFrame(void);
