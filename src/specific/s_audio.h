#pragma once

#include <SDL2/SDL_audio.h>
#include <libavutil/samplefmt.h>
#include <stdbool.h>
#include <stddef.h>

#define AUDIO_MAX_SAMPLES 1000
#define AUDIO_MAX_ACTIVE_SAMPLES 50
#define AUDIO_MAX_ACTIVE_STREAMS 10
#define AUDIO_NO_SOUND (-1)

bool S_Audio_Init(void);
bool S_Audio_Shutdown(void);

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

bool S_Audio_SamplesClear(void);
bool S_Audio_SamplesLoad(size_t count, const char **contents, size_t *sizes);

int S_Audio_SampleSoundPlay(
    int sample_id, int volume, float pitch, int pan, bool is_looped);
bool S_Audio_SampleSoundIsPlaying(int sound_id);
bool S_Audio_SampleSoundClose(int sound_id);
bool S_Audio_SampleSoundCloseAll(void);
bool S_Audio_SampleSoundSetPan(int sound_id, int pan);
bool S_Audio_SampleSoundSetVolume(int sound_id, int volume);

#ifdef S_AUDIO_IMPL
    #include <libavformat/avformat.h>
    #include <SDL2/SDL.h>

    #define AUDIO_WORKING_RATE 44100
    #define AUDIO_WORKING_FORMAT AUDIO_F32
    #define AUDIO_SAMPLES 500
    #define AUDIO_WORKING_CHANNELS 2

extern SDL_AudioDeviceID g_AudioDeviceID;

int S_Audio_GetAVAudioFormat(const int sample_fmt);
int S_Audio_GetSDLAudioFormat(const enum AVSampleFormat sample_fmt);

void S_Audio_SampleSoundInit(void);
void S_Audio_SampleSoundShutdown(void);
void S_Audio_SampleSoundMix(float *dst_buffer, size_t len);

void S_Audio_StreamSoundInit(void);
void S_Audio_StreamSoundShutdown(void);
void S_Audio_StreamSoundMix(float *dst_buffer, size_t len);

#endif
