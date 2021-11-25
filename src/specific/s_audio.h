#ifndef T1M_SPECIFIC_S_AUDIO_H
#define T1M_SPECIFIC_S_AUDIO_H

#include <stdbool.h>

#define AUDIO_NO_SOUND (-1)

bool S_Audio_Init();

bool S_Audio_StreamSoundPause(int sound_id);
bool S_Audio_StreamSoundUnpause(int sound_id);
int S_Audio_StreamSoundCreateFromFile(const char *path);
bool S_Audio_StreamSoundClose(int sound_id);
bool S_Audio_StreamSoundIsLooped(int sound_id);
bool S_Audio_StreamSoundSetVolume(int sound_id, float volume);
bool S_Audio_StreamSoundSetIsLooped(int sound_id, bool is_looped);
bool S_Audio_StreamSoundSetFinishCallback(
    int sound_id, void (*callback)(int sound_id, void *user_data),
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

void S_Audio_StreamSoundInit();
void S_Audio_StreamSoundMix(float *target_buffer, size_t len);
#endif

#endif
