#pragma once

#include "../math.h"
#include "../types.h"
#include "enum.h"
#include "ids.h"
#include "types.h"

#include <stddef.h>

#define SOUND_DEFAULT_PITCH 0x10000

bool Sound_Init(void);
void Sound_Shutdown(void);
bool Sound_IsInitialised(void);

void Sound_SetMasterVolume(float volume);

void Sound_ResetSamples(void);

void Sound_InitialiseSampleInfos(int32_t num_sample_infos);
int32_t Sound_GetSampleCount(void);
bool Sound_LoadSampleData(
    int32_t sample_data_id, const char *sample_data, size_t size);

void Sound_InitialiseSources(int32_t num_sources);
int32_t Sound_GetSourceCount(void);
OBJECT_VECTOR *Sound_GetSource(int32_t source_idx);
void Sound_ResetSources(void);

int16_t *Sound_GetSampleLUT(void);
SAMPLE_INFO *Sound_GetSampleInfo(SAMPLE_ID sfx_num);
SAMPLE_INFO *Sound_GetSampleInfoByIdx(int32_t info_idx);

bool Sound_IsAvailable(SAMPLE_ID sfx_num);
bool Sound_Effect(SAMPLE_ID sfx_num, const XYZ_32 *pos, uint32_t flags);
void Sound_StopEffect(SAMPLE_ID sfx_num);

void Sound_ResetAmbient(void);
void Sound_UpdateEffects(void);

void Sound_PauseAll(void);
void Sound_UnpauseAll(void);
void Sound_StopAll(void);
