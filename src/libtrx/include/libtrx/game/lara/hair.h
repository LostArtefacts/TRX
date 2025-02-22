#pragma once

#include "../math.h"

typedef struct {
    XYZ_32 pos;
    XYZ_16 rot;
    struct {
        struct {
            XYZ_32 pos;
            XYZ_16 rot;
        } result, prev;
    } interp;
} HAIR_SEGMENT;

extern void Lara_Hair_Initialise(void);
extern bool Lara_Hair_IsActive(void);
extern void Lara_Hair_Draw(void);

extern int32_t Lara_Hair_GetSegmentCount(void);
extern HAIR_SEGMENT *Lara_Hair_GetSegment(int32_t n);
