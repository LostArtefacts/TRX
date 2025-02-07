#pragma once

#include "../math.h"
#include "../types.h"
#include "enum.h"
#include "ids.h"

void Sound_InitialiseSources(int32_t num_sources);
int32_t Sound_GetSourceCount(void);
OBJECT_VECTOR *Sound_GetSource(int32_t source_idx);

void Sound_PauseAll(void);
void Sound_UnpauseAll(void);

extern void Sound_StopAll(void);
extern bool Sound_Effect(
    SOUND_EFFECT_ID sfx_num, const XYZ_32 *pos, uint32_t flags);
extern bool Sound_IsAvailable(SOUND_EFFECT_ID sfx_num);
