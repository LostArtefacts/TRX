#define S_AUDIO_IMPL
#include "specific/s_audio.h"

#include "memory.h"
#include "log.h"

#include <assert.h>

SDL_AudioDeviceID g_AudioDeviceID = 0;
static size_t m_WorkingBufferSize = 0;
static float *m_WorkingBuffer = NULL;
static Uint8 m_WorkingSilence = 0;

static void S_Audio_MixerCallback(void *userdata, Uint8 *stream_data, int len);

static void S_Audio_MixerCallback(void *userdata, Uint8 *stream_data, int len)
{
    memset(m_WorkingBuffer, m_WorkingSilence, len);
    S_Audio_StreamSoundMix(m_WorkingBuffer, len);
    S_Audio_SampleSoundMix(m_WorkingBuffer, len);
    memcpy(stream_data, m_WorkingBuffer, len);
}

bool S_Audio_Init()
{
    if (g_AudioDeviceID) {
        // already initialized
        return true;
    }

    int32_t result = SDL_Init(SDL_INIT_AUDIO);
    if (result < 0) {
        LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
        return false;
    }

    S_Audio_SampleSoundInit();
    S_Audio_StreamSoundInit();

    SDL_AudioSpec desired;
    SDL_memset(&desired, 0, sizeof(desired));
    desired.freq = AUDIO_WORKING_RATE;
    desired.format = AUDIO_WORKING_FORMAT;
    desired.channels = AUDIO_WORKING_CHANNELS;
    desired.samples = AUDIO_SAMPLES;
    desired.callback = S_Audio_MixerCallback;
    desired.userdata = NULL;

    SDL_AudioSpec delivered;
    g_AudioDeviceID = SDL_OpenAudioDevice(NULL, 0, &desired, &delivered, 0);

    if (!g_AudioDeviceID) {
        LOG_ERROR("Failed to open audio device: %s", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(g_AudioDeviceID, 0);

    m_WorkingSilence = desired.silence;
    m_WorkingBufferSize = desired.samples * desired.channels
        * SDL_AUDIO_BITSIZE(desired.format) / 8;

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_WorkingBuffer = Memory_Alloc(m_WorkingBufferSize);
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

bool S_Audio_Shutdown()
{
    if (!g_AudioDeviceID) {
        return true;
    }

    S_Audio_SampleSoundShutdown();
    S_Audio_StreamSoundShutdown();

    SDL_PauseAudioDevice(g_AudioDeviceID, 1);
    SDL_CloseAudioDevice(g_AudioDeviceID);
    g_AudioDeviceID = 0;

    return true;
}
