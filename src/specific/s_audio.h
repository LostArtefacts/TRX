#ifndef T1M_SPECIFIC_S_AUDIO_H
#define T1M_SPECIFIC_S_AUDIO_H

#include <stdbool.h>

bool S_Audio_Init();

bool S_Audio_StreamPause(int stream_id);
bool S_Audio_StreamUnpause(int stream_id);
int S_Audio_StreamCreateFromFile(const char *path);
bool S_Audio_StreamClose(int stream_id);
bool S_Audio_StreamIsLooped(int stream_id);
bool S_Audio_StreamSetVolume(int stream_id, float volume);
bool S_Audio_StreamSetIsLooped(int stream_id, bool is_looped);
bool S_Audio_StreamSetFinishCallback(
    int stream_id, void (*callback)(int stream_id, void *user_data),
    void *user_data);

#ifdef S_AUDIO_IMPL
    #include <SDL2/SDL.h>

    #define AUDIO_WORKING_RATE 44100
    #define AUDIO_WORKING_FORMAT AUDIO_F32
    #define AUDIO_SAMPLES 4410
    #define AUDIO_WORKING_CHANNELS 2

extern SDL_AudioDeviceID g_AudioDeviceID;

float S_Audio_Clamp(float min, float max, float val);
float S_Audio_InverseLerp(float from, float to, float val);

void S_Audio_StreamInit();
void S_Audio_StreamProcessSamples(float *target_buffer, size_t len);
#endif

#endif
