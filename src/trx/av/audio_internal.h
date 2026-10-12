#pragma once

#include <trx/av/audio.h>

#include <SDL2/SDL_audio.h>
#include <libavformat/avformat.h>

#define AUDIO_WORKING_RATE 44100
#define AUDIO_WORKING_FORMAT AUDIO_F32
#define AUDIO_SAMPLES 500
#define AUDIO_WORKING_CHANNELS 2
#define AUDIO_FILTER_STAGES 2

// A low-pass filter state for one interleaved stream. The cutoff glides to a
// new value instead of jumping, so that changes do not click.
typedef struct {
    bool is_active;
    float cutoff_log;
    float target_log;
    float gain;
    float band[AUDIO_FILTER_STAGES][AUDIO_WORKING_CHANNELS];
    float low[AUDIO_FILTER_STAGES][AUDIO_WORKING_CHANNELS];
} AUDIO_FILTER;

extern SDL_AudioDeviceID g_AudioDeviceID;

// Reports an audio device that is not open.
RESULT Audio_CheckDevice(void);

void Audio_LockDevice(void);
void Audio_UnlockDevice(void);

// Guards every decoder-side structure: libav contexts, resamplers, and the
// buffers the mixer reads from. The audio callback must never take it, as it
// is held across decoding.
void Audio_WorkerLock(void);
void Audio_WorkerUnlock(void);

int32_t Audio_GetAVChannelLayout(int32_t sample_fmt);
int32_t Audio_GetAVAudioFormat(int32_t sample_fmt);
int32_t Audio_GetSDLAudioFormat(enum AVSampleFormat sample_fmt);

void Audio_Sample_Init(void);
void Audio_Sample_Shutdown(void);
void Audio_Sample_Mix(float *dst_buffer, size_t len);

// Decode samples that have started playing. Runs on the worker thread.
void Audio_Sample_Pump(void);

void Audio_Stream_Init(void);
void Audio_Stream_Shutdown(void);
void Audio_Stream_Mix(float *dst_buffer, size_t len);

// Decode ahead of the mixer. Runs on the worker thread.
void Audio_Stream_Pump(void);

// Opens the filter fully and clears its state.
void Audio_Filter_Reset(AUDIO_FILTER *filter);
// Sets the frequency, in Hz, above which the filter cuts. Zero or less opens
// the filter fully.
void Audio_Filter_SetCutoff(AUDIO_FILTER *filter, float cutoff);
// Filters the interleaved samples in place. An open filter leaves them
// untouched.
void Audio_Filter_Process(AUDIO_FILTER *filter, float *buffer, uint32_t count);

void Audio_Reverb_Init(int32_t sample_rate, int32_t channels);
void Audio_Reverb_Shutdown(void);
void Audio_Reverb_Process(float *dst_buffer, size_t len);
void Audio_Reverb_SetType(uint8_t reverb_type);
uint8_t Audio_Reverb_GetType(void);
