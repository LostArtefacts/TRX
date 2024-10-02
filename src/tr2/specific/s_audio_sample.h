#pragma once

#include "global/types.h"

const SOUND_ADAPTER_NODE *__cdecl S_Audio_Sample_GetAdapter(const GUID *guid);
void __cdecl S_Audio_Sample_CloseAllTracks(void);
bool __cdecl S_Audio_Sample_Load(
    int32_t sample_id, LPWAVEFORMATEX format, const void *data,
    uint32_t data_size);
bool __cdecl S_Audio_Sample_IsTrackPlaying(int32_t track_id);
int32_t __cdecl S_Audio_Sample_Play(
    int32_t sample_id, int32_t volume, int32_t pitch, int32_t pan,
    uint32_t flags);
int32_t __cdecl S_Audio_Sample_GetFreeTrackIndex(void);
void __cdecl S_Audio_Sample_AdjustTrackVolumeAndPan(
    int32_t track_id, int32_t volume, int32_t pan);
void __cdecl S_Audio_Sample_AdjustTrackPitch(int32_t track_id, int32_t pitch);
void __cdecl S_Audio_Sample_CloseTrack(int32_t track_id);
bool __cdecl S_Audio_Sample_Init(void);
bool __cdecl S_Audio_Sample_DSoundEnumerate(SOUND_ADAPTER_LIST *adapter_list);
BOOL CALLBACK S_Audio_Sample_DSoundEnumCallback(
    LPGUID guid, LPCTSTR description, LPCTSTR module, LPVOID context);
void __cdecl S_Audio_Sample_Init2(HWND hwnd);
bool __cdecl S_Audio_Sample_DSoundCreate(GUID *guid);
bool __cdecl S_Audio_Sample_DSoundBufferTest(void);
void __cdecl S_Audio_Sample_Shutdown(void);
bool __cdecl S_Audio_Sample_IsEnabled(void);
int32_t __cdecl S_Audio_Sample_OutPlay(
    int32_t sample_id, int32_t volume, int32_t pitch, int32_t pan);
int32_t __cdecl S_Audio_Sample_CalculateSampleVolume(int32_t volume);
int32_t __cdecl S_Audio_Sample_CalculateSamplePan(int16_t pan);
int32_t __cdecl S_Audio_Sample_OutPlayLooped(
    int32_t sample_id, int32_t volume, int32_t pitch, int32_t pan);
void __cdecl S_Audio_Sample_OutSetPanAndVolume(
    int32_t track_id, int32_t pan, int32_t volume);
void __cdecl S_Audio_Sample_OutSetPitch(int32_t track_id, int32_t pitch);
void __cdecl S_Audio_Sample_OutCloseTrack(int32_t track_id);
void __cdecl S_Audio_Sample_OutCloseAllTracks(void);
bool __cdecl S_Audio_Sample_OutIsTrackPlaying(int32_t track_id);
