#ifndef T1M_SPECIFIC_S_AUDIO_H
#define T1M_SPECIFIC_S_AUDIO_H

#include <stdbool.h>

typedef struct SOUND_STREAM SOUND_STREAM;

bool S_Audio_Init();
bool S_Audio_PauseStreaming(int sound_id);
bool S_Audio_UnpauseStreaming(int sound_id);
int S_Audio_StartStreaming(const char *path);
bool S_Audio_StopStreaming(int sound_id);
bool S_Audio_IsStreamLooped(int sound_id);
bool S_Audio_SetStreamVolume(int sound_id, float volume);
bool S_Audio_SetStreamIsLooped(int sound_id, bool is_looped);
bool S_Audio_SetStreamFinishCallback(
    int sound_id, void (*callback)(int sound_id, void *user_data),
    void *user_data);

#endif
