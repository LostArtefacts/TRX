#pragma once

#include <trx/core/math.h>
#include <trx/game/lara/skin/types.h>
#include <trx/game/objects/types.h>

typedef struct {
    XYZ_32 pos;
    XYZ_16 rot;
    XYZ_32 vel;
    struct {
        struct {
            XYZ_32 pos;
            XYZ_16 rot;
        } result, prev;
    } interp;
} HAIR_SEGMENT;

// Resets every braid segment to its bone offset, hanging straight down at rest,
// and marks the chain as not yet built. The braid stays undrawn until the next
// control pass chains it off her head.
void Lara_Hair_Initialise(void);

// Finds the ring shared by adjacent braid segments and enables welding it shut
// at draw time. A no-op unless the outfit opts into joints and the segment
// meshes share a ring; call whenever the outfit changes.
void Lara_Hair_InitJoints(const LARA_SKIN_OUTFIT *outfit);

// Rebuilds the braid chain when the outfit brings a different braid. A no-op
// while the pigtail count, segment meshes, and bones stay the same; call
// whenever the outfit changes.
void Lara_Hair_Rebuild(void);

bool Lara_Hair_IsActive(void);

void Lara_Hair_Control(bool in_cutscene);

void Lara_Hair_Draw(void);

int32_t Lara_Hair_GetBraidCount(void);

int32_t Lara_Hair_GetSegmentCount(void);

HAIR_SEGMENT *Lara_Hair_GetSegment(int32_t braid_idx, int32_t segment_idx);
