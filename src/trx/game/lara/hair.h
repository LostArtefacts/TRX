#pragma once

#include <trx/core/math.h>
#include <trx/game/lara/skin/types.h>
#include <trx/game/objects/types.h>

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

void Lara_Hair_Initialise(void);
// Detects the ring adjacent braid segments share and enables welding it shut at
// draw time. A no-op unless the outfit opts into joints and the segment meshes
// actually share a ring; call whenever the outfit changes.
void Lara_Hair_InitJoints(const LARA_SKIN_OUTFIT *outfit);
bool Lara_Hair_IsActive(void);
void Lara_Hair_Control(bool in_cutscene);
void Lara_Hair_Draw(void);

int32_t Lara_Hair_GetBraidCount(void);
int32_t Lara_Hair_GetSegmentCount(void);
HAIR_SEGMENT *Lara_Hair_GetSegment(int32_t braid_idx, int32_t segment_idx);
