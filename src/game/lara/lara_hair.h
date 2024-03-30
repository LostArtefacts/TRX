#pragma once

#include "global/types.h"

typedef struct HAIR_SEGMENT {
    XYZ_32 pos;
    XYZ_16 rot;
    struct {
        struct {
            XYZ_32 pos;
            XYZ_16 rot;
        } result, prev;
    } interp;
} HAIR_SEGMENT;

bool Lara_Hair_IsActive(void);
void Lara_Hair_Initialise(void);
void Lara_Hair_SetLaraType(GAME_OBJECT_ID lara_type);
void Lara_Hair_Control(void);
void Lara_Hair_Draw(void);

int32_t Lara_Hair_GetSegmentCount(void);
HAIR_SEGMENT *Lara_Hair_GetSegment(int32_t n);
