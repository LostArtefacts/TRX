#ifndef T1M_SPECIFIC_S_AUDIO_H
#define T1M_SPECIFIC_S_AUDIO_H

#include <stdbool.h>

bool S_Audio_Init();
bool S_Audio_PauseStreaming(int sound_id);
bool S_Audio_UnpauseStreaming(int sound_id);
int S_Audio_StartStreaming(const char *path);
bool S_Audio_StopStreaming(int sound_id);
bool S_Audio_SetStreamVolume(int sound_id, float volume);

#endif
