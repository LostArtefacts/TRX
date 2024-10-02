#pragma once

#include "global/types.h"

void __cdecl Sound_Init(void);
void __cdecl Sound_Shutdown(void);

void __cdecl Sound_SetMasterVolume(int32_t volume);
void __cdecl Sound_UpdateEffects(void);
void __cdecl Sound_Effect(
    SOUND_EFFECT_ID sample_id, const XYZ_32 *pos, uint32_t flags);
void __cdecl Sound_StopEffect(SOUND_EFFECT_ID sample_id);
void __cdecl Sound_StopAllSamples(void);
void __cdecl Sound_EndScene(void);
