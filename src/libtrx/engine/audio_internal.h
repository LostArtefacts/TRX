#pragma once

#include "engine/audio.h"

#include <SDL2/SDL.h>
#include <libavformat/avformat.h>

#define AUDIO_WORKING_RATE 44100
#define AUDIO_WORKING_FORMAT AUDIO_F32
#define AUDIO_SAMPLES 500
#define AUDIO_WORKING_CHANNELS 2

extern SDL_AudioDeviceID g_AudioDeviceID;

int32_t Audio_GetAVChannelLayout(int32_t sample_fmt);
int32_t Audio_GetAVAudioFormat(int32_t sample_fmt);
int32_t Audio_GetSDLAudioFormat(enum AVSampleFormat sample_fmt);

void Audio_Sample_Init(void);
void Audio_Sample_Shutdown(void);
void Audio_Sample_Mix(float *dst_buffer, size_t len);

void Audio_Stream_Init(void);
void Audio_Stream_Shutdown(void);
void Audio_Stream_Mix(float *dst_buffer, size_t len);
