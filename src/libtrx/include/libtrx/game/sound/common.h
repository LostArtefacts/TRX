#pragma once

#include "../math.h"
#include "../types.h"
#include "enum.h"
#include "ids.h"
#include "types.h"

void Sound_InitialiseSources(int32_t num_sources);
void Sound_InitialiseSampleInfos(int32_t num_sample_infos);
int32_t Sound_GetSourceCount(void);
OBJECT_VECTOR *Sound_GetSource(int32_t source_idx);
int16_t *Sound_GetSampleLUT(void);
SAMPLE_INFO *Sound_GetSampleInfo(SOUND_EFFECT_ID sfx_num);
SAMPLE_INFO *Sound_GetSampleInfoByIdx(int32_t info_idx);

void Sound_ResetSources(void);
void Sound_PauseAll(void);
void Sound_UnpauseAll(void);

extern void Sound_StopAll(void);
extern bool Sound_Effect(
    SOUND_EFFECT_ID sfx_num, const XYZ_32 *pos, uint32_t flags);
bool Sound_IsAvailable(SOUND_EFFECT_ID sfx_num);
