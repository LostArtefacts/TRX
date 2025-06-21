#include "audio_internal.h"

#include "log.h"
#include "memory.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_stdinc.h>
#include <stdint.h>
#include <string.h>

SDL_AudioDeviceID g_AudioDeviceID = 0;
static int32_t m_RefCount = 0;
static size_t m_MixBufferCapacity = 0;
static float *m_MixBuffer = nullptr;
static Uint8 m_Silence = 0;

static void M_MixerCallback(void *userdata, Uint8 *stream_data, int32_t len);

static void M_MixerCallback(void *userdata, Uint8 *stream_data, int32_t len)
{
    memset(m_MixBuffer, m_Silence, len);
    Audio_Stream_Mix(m_MixBuffer, len);
    Audio_Sample_Mix(m_MixBuffer, len);
    memcpy(stream_data, m_MixBuffer, len);
}

bool Audio_Init(void)
{
    m_RefCount++;
    if (g_AudioDeviceID) {
        // already initialized
        return true;
    }

    int32_t result = SDL_Init(SDL_INIT_AUDIO);
    if (result < 0) {
        LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
        return false;
    }

    SDL_AudioSpec desired;
    SDL_memset(&desired, 0, sizeof(desired));
    desired.freq = AUDIO_WORKING_RATE;
    desired.format = AUDIO_WORKING_FORMAT;
    desired.channels = AUDIO_WORKING_CHANNELS;
    desired.samples = AUDIO_SAMPLES;
    desired.callback = M_MixerCallback;
    desired.userdata = nullptr;

    SDL_AudioSpec delivered;
    g_AudioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desired, &delivered, 0);

    if (!g_AudioDeviceID) {
        LOG_ERROR("Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    m_Silence = desired.silence;
    m_MixBufferCapacity = desired.samples * desired.channels
        * SDL_AUDIO_BITSIZE(desired.format) / 8;

    m_MixBuffer = Memory_Alloc(m_MixBufferCapacity);

    SDL_PauseAudioDevice(g_AudioDeviceID, 0);

    Audio_Sample_Init();
    Audio_Stream_Init();

    return true;
}

bool Audio_Shutdown(void)
{
    m_RefCount--;
    if (m_RefCount > 0) {
        return false;
    }

    if (g_AudioDeviceID) {
        SDL_PauseAudioDevice(g_AudioDeviceID, 1);
        SDL_CloseAudioDevice(g_AudioDeviceID);
        g_AudioDeviceID = 0;
    }

    Memory_FreePointer(&m_MixBuffer);

    Audio_Sample_Shutdown();
    Audio_Stream_Shutdown();
    return true;
}

int32_t Audio_GetAVChannelLayout(const int32_t channels)
{
    switch (channels) {
        // clang-format off
        case 1: return AV_CH_LAYOUT_MONO;
        case 2: return AV_CH_LAYOUT_STEREO;
        default: return AV_CH_LAYOUT_MONO;
        // clang-format on
    }
}

int32_t Audio_GetAVAudioFormat(const int32_t sample_fmt)
{
    switch (sample_fmt) {
        // clang-format off
        case AUDIO_U8: return AV_SAMPLE_FMT_U8;
        case AUDIO_S16: return AV_SAMPLE_FMT_S16;
        case AUDIO_S32: return AV_SAMPLE_FMT_S32;
        case AUDIO_F32: return AV_SAMPLE_FMT_FLT;
        default: return -1;
        // clang-format on
    }
}

int32_t Audio_GetSDLAudioFormat(const enum AVSampleFormat sample_fmt)
{
    // clang-format off
    switch (sample_fmt) {
        case AV_SAMPLE_FMT_U8: return AUDIO_U8;
        case AV_SAMPLE_FMT_S16: return AUDIO_S16;
        case AV_SAMPLE_FMT_S32: return AUDIO_S32;
        case AV_SAMPLE_FMT_FLT: return AUDIO_F32;
        default: return -1;
    }
    // clang-format on
}
