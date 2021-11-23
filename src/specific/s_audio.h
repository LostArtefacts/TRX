#ifndef T1M_SPECIFIC_S_AUDIO_H
#define T1M_SPECIFIC_S_AUDIO_H

#include <stdbool.h>

bool S_Audio_Init();
bool S_Audio_StreamPause(int stream_id);
bool S_Audio_StreamUnpause(int stream_id);
int S_Audio_StreamCreateFromFile(const char *path);
bool S_Audio_StreamStop(int stream_id);
bool S_Audio_StreamIsLooped(int stream_id);
bool S_Audio_StreamSetVolume(int stream_id, float volume);
bool S_Audio_StreamSetIsLooped(int stream_id, bool is_looped);
bool S_Audio_StreamSetFinishCallback(
    int stream_id, void (*callback)(int stream_id, void *user_data),
    void *user_data);

#endif
