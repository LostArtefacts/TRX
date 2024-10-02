#pragma once

#include "global/types.h"

bool __cdecl Music_Init(void);
void __cdecl Music_Shutdown(void);
void __cdecl Music_Play(int16_t track_id, bool is_looped);
void __cdecl Music_Stop(void);
bool __cdecl Music_PlaySynced(int16_t track_id);
double __cdecl Music_GetTimestamp(void);
void __cdecl Music_SetVolume(int32_t volume);
