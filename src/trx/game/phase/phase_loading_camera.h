#pragma once

#include <trx/game/phase/types.h>
#include <trx/game/types.h>

typedef struct {
    XYZ_32 source;
    XYZ_32 target;
    int16_t room_num;
    double display_time;
    double fade_in_time;
    double fade_out_time;
} PHASE_LOADING_CAMERA_ARGS;

PHASE *Phase_LoadingCamera_Create(PHASE_LOADING_CAMERA_ARGS args);
void Phase_LoadingCamera_Destroy(PHASE *phase);
