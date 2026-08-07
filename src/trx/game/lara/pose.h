#pragma once

#include <trx/game/lara/enum.h>
#include <trx/game/types.h>

typedef struct {
    XYZ_16 offset;
    XYZ_16 rots[LM_NUMBER_OF];
} LARA_POSE;

bool Lara_Pose_IsAvailable(void);
void Lara_Pose_Clear(void);
void Lara_Pose_Cycle(int32_t dir);
const LARA_POSE *Lara_Pose_Get(void);

// Forces Lara into the given pose regardless of the photo mode pose state;
// used by the TR4 cutscene playback. Pass nullptr to release.
void Lara_Pose_SetOverride(const LARA_POSE *pose);
