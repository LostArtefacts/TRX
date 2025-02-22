#pragma once

#include "global/types.h"

typedef enum {
    LF_FL_HOLD_FT = 1,
    LF_FL_THROW_FT = 32,
    LF_FL_DRAW_FT = 39,
    LF_FL_IGNITE_FT = 23,
    LF_FL_2_HOLD_FT = 15,

    LF_FL_HOLD = 0,
    LF_FL_THROW = (LF_FL_HOLD + LF_FL_HOLD_FT), // = 1
    LF_FL_THROW_RELEASE = (LF_FL_THROW + 20), // = 21
    LF_FL_DRAW = (LF_FL_THROW + LF_FL_THROW_FT), // = 33
    LF_FL_IGNITE = (LF_FL_DRAW + LF_FL_DRAW_FT), // = 72
    LF_FL_2_HOLD = (LF_FL_IGNITE + LF_FL_IGNITE_FT), // = 95
    LF_FL_END = (LF_FL_2_HOLD + LF_FL_2_HOLD_FT), // = 110
    LF_FL_DRAW_GOT_IT = (LF_FL_DRAW + 13), // = 46
} LARA_FLARE_ANIMATION_FRAME;

int32_t Flare_DoLight(const XYZ_32 *pos, int32_t flare_age);
void Flare_DoInHand(int32_t flare_age);
void Flare_DrawInAir(const ITEM *item);
void Flare_Create(bool thrown);
void Flare_SetArm(int32_t frame);
void Flare_Draw(void);
void Flare_Undraw(void);
void Flare_DrawMeshes(void);
void Flare_UndrawMeshes(void);
void Flare_Ready(void);
void Flare_Control(int16_t item_num);
