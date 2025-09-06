#pragma once

#include "../math.h"
#include "../types.h"
#include "enum.h"
#include "ids.h"
#include "types.h"

#include <stddef.h>

#define SOUND_DEFAULT_PITCH 0x10000

int32_t Sound_ConvertVolumeToDecibel(int32_t volume);
int32_t Sound_ConvertPanToDecibel(uint16_t pan);

extern bool Sound_Init(void);
bool Sound_InitialiseCommon(void);
void Sound_Shutdown(void);
bool Sound_IsInitialised(void);

// Stops and unloads all samples
extern void Sound_Reset(void);

void Sound_InitialiseSources(int32_t num_sources);
void Sound_InitialiseSampleInfos(int32_t num_sample_infos);
bool Sound_LoadSample(int32_t sample_num, const char *sample_data, size_t size);
int32_t Sound_GetSourceCount(void);
OBJECT_VECTOR *Sound_GetSource(int32_t source_idx);
int16_t *Sound_GetSampleLUT(void);
int32_t Sound_GetSampleCount(void);
SAMPLE_INFO *Sound_GetSampleInfo(SAMPLE_ID sfx_num);
SAMPLE_INFO *Sound_GetSampleInfoByIdx(int32_t info_idx);

void Sound_SetMasterVolume(float volume);

void Sound_ResetSources(void);
void Sound_PauseAll(void);
void Sound_UnpauseAll(void);

extern void Sound_StopAll(void);
extern void Sound_StopAmbientSounds(void);
extern bool Sound_Effect(SAMPLE_ID sfx_num, const XYZ_32 *pos, uint32_t flags);
extern void Sound_StopEffect(SAMPLE_ID sfx_num);
extern void Sound_UpdateEffects(void);

bool Sound_IsAvailable(SAMPLE_ID sfx_num);
