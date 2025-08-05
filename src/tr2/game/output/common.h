#pragma once

#include "global/types.h"

#include <libtrx/game/output.h>

#include <stdint.h>

#define SPRITE_ABS 0x01000000
#define SPRITE_SEMI_TRANS 0x02000000
#define SPRITE_SHADE 0x08000000

typedef struct {
    struct {
        float x;
        float y;
        float z;
    } pos;
    float rhw;
    struct {
        float u;
        float v;
        float z;
        float w;
    } tex;
    float g;
} VERTEX_INFO;

void Output_LoadBackgroundFromObject(void);

void Output_SetSunsetEnabled(bool enabled);
void Output_SetSunsetTimer(int32_t timer);
int32_t Output_GetSunsetTimer(void);
int32_t Output_GetSunsetDuration(void);

void Output_ApplyRenderSettings(void);
