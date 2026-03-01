#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "vendor/dr_wav.h"

#include <trx/av/audio_internal.h>

#include <trx/core/benchmark.h>
#include <trx/debug.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/version.h>

#include <SDL2/SDL_audio.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *original_data;
    size_t original_size;
    float *sample_data;
    int32_t channels;
    int32_t num_samples;
} AUDIO_SAMPLE;

typedef struct {
    bool is_used;
    bool is_looped;
    bool is_playing;
    float volume_l;
    float volume_r;

    float pitch;
    int32_t volume;
    int32_t pan;

    float current_sample;

    AUDIO_SAMPLE *sample;
} AUDIO_SAMPLE_SOUND;

static int32_t m_LoadedSamplesCount = 0;
static AUDIO_SAMPLE m_LoadedSamples[AUDIO_MAX_SAMPLES] = {};
static AUDIO_SAMPLE_SOUND m_Samples[AUDIO_MAX_ACTIVE_SAMPLES] = {};

static double M_DecibelToMultiplier(double db_gain)
{
    if (g_TRVersion < 3) {
        return pow(2.0, db_gain / 600.0);
    } else {
        return pow(10.0, db_gain / 2000.0);
    }
}

static bool M_RecalculateChannelVolumes(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
    sound->volume_l = M_DecibelToMultiplier(
        sound->volume - (sound->pan > 0 ? sound->pan : 0));
    sound->volume_r = M_DecibelToMultiplier(
        sound->volume + (sound->pan < 0 ? sound->pan : 0));

    return true;
}

static bool M_ConvertSample(const int32_t sample_id)
{
    ASSERT(sample_id >= 0 && sample_id < m_LoadedSamplesCount);
    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    if (sample->sample_data != nullptr) {
        return true;
    }

    BENCHMARK benchmark = Benchmark_Start();

    drwav wav;
    if (!drwav_init_memory(
            &wav, sample->original_data, sample->original_size, nullptr)) {
        LOG_ERROR("Failed to decode WAV sample %d", sample_id);
        return false;
    }

    const drwav_uint64 total_frames = wav.totalPCMFrameCount;
    const int32_t src_channels = (int32_t)wav.channels;
    const int32_t src_rate = (int32_t)wav.sampleRate;

    float *raw_pcm = Memory_Alloc(total_frames * src_channels * sizeof(float));
    const drwav_uint64 frames_read =
        drwav_read_pcm_frames_f32(&wav, total_frames, raw_pcm);
    drwav_uninit(&wav);

    if (frames_read == 0) {
        LOG_ERROR("Failed to read PCM frames from sample %d", sample_id);
        Memory_FreePointer(&raw_pcm);
        return false;
    }

    // Target: mono, 44100 Hz (matches AUDIO_WORKING_RATE)
    if (src_rate == AUDIO_WORKING_RATE && src_channels == 1) {
        sample->sample_data = raw_pcm;
        sample->channels = 1;
        sample->num_samples = (int32_t)frames_read;
    } else {
        SDL_AudioStream *cvt = SDL_NewAudioStream(
            AUDIO_F32SYS, src_channels, src_rate, AUDIO_F32SYS, 1,
            AUDIO_WORKING_RATE);
        if (cvt == nullptr) {
            LOG_ERROR(
                "Failed to create SDL_AudioStream for sample %d: %s", sample_id,
                SDL_GetError());
            Memory_FreePointer(&raw_pcm);
            return false;
        }

        SDL_AudioStreamPut(
            cvt, raw_pcm,
            (int32_t)(frames_read * src_channels * sizeof(float)));
        SDL_AudioStreamFlush(cvt);
        Memory_FreePointer(&raw_pcm);

        const int32_t avail = SDL_AudioStreamAvailable(cvt);
        if (avail <= 0) {
            LOG_ERROR(
                "SDL_AudioStream produced no output for sample %d", sample_id);
            SDL_FreeAudioStream(cvt);
            return false;
        }

        float *resampled = Memory_Alloc(avail);
        const int32_t got = SDL_AudioStreamGet(cvt, resampled, avail);
        SDL_FreeAudioStream(cvt);

        if (got <= 0) {
            LOG_ERROR("SDL_AudioStreamGet failed for sample %d", sample_id);
            Memory_FreePointer(&resampled);
            return false;
        }

        sample->sample_data = resampled;
        sample->channels = 1;
        sample->num_samples = got / (int32_t)sizeof(float);
    }

    char buffer[80];
    sprintf(buffer, "sample %d decoded", sample_id);
    Benchmark_End(&benchmark, buffer);

    return true;
}

void Audio_Sample_Init(void)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
        sound->is_used = false;
        sound->is_playing = false;
        sound->volume = 0.0f;
        sound->pitch = 1.0f;
        sound->pan = 0.0f;
        sound->current_sample = 0.0f;
        sound->sample = nullptr;
    }
}

void Audio_Sample_Shutdown(void)
{
    if (!g_AudioDeviceID) {
        return;
    }

    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();
}

bool Audio_Sample_Unload(const int32_t sample_id)
{
    if (!g_AudioDeviceID) {
        LOG_ERROR("Unitialized audio device");
        return false;
    }

    if (sample_id < 0 || sample_id >= AUDIO_MAX_SAMPLES) {
        LOG_ERROR("Maximum allowed samples: %d", AUDIO_MAX_SAMPLES);
        return false;
    }

    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    if (sample->sample_data == nullptr) {
        LOG_ERROR("Sample %d is already unloaded", sample_id);
        return false;
    }
    Memory_FreePointer(&sample->sample_data);
    Memory_FreePointer(&sample->original_data);
    m_LoadedSamplesCount--;
    return true;
}

bool Audio_Sample_UnloadAll(void)
{
    if (!g_AudioDeviceID) {
        LOG_ERROR("Unitialized audio device");
        return false;
    }

    m_LoadedSamplesCount = 0;
    for (int32_t i = 0; i < AUDIO_MAX_SAMPLES; i++) {
        AUDIO_SAMPLE *const sample = &m_LoadedSamples[i];
        Memory_FreePointer(&sample->sample_data);
        Memory_FreePointer(&sample->original_data);
    }
    return true;
}

bool Audio_Sample_Load(
    const int32_t sample_id, const char *const data, const size_t size)
{
    if (data == nullptr || size == 0) {
        LOG_ERROR("Missing sample data %d", sample_id);
        return false;
    }

    if (!g_AudioDeviceID) {
        LOG_ERROR("Unitialized audio device");
        return false;
    }

    if (sample_id < 0 || sample_id >= AUDIO_MAX_SAMPLES) {
        LOG_ERROR("Maximum allowed samples: %d", AUDIO_MAX_SAMPLES);
        return false;
    }

    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    if (sample->original_data != nullptr) {
        LOG_ERROR(
            "Sample %d is already loaded (trying to overwrite with %d bytes)",
            sample_id, size);
        return false;
    }

    sample->original_data = Memory_Alloc(size);
    sample->original_size = size;
    memcpy(sample->original_data, data, size);
    m_LoadedSamplesCount++;
    return true;
}

int32_t Audio_Sample_Play(
    int32_t sample_id, int32_t volume, float pitch, int32_t pan, bool is_looped)
{
    if (!g_AudioDeviceID) {
        LOG_ERROR("audio device is unavailable");
        return false;
    }

    if (sample_id < 0 || sample_id >= m_LoadedSamplesCount) {
        LOG_DEBUG("Invalid sample id: %d", sample_id);
        return AUDIO_NO_SOUND;
    }

    int32_t result = AUDIO_NO_SOUND;

    Audio_LockDevice();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
        if (sound->is_used) {
            continue;
        }

        M_ConvertSample(sample_id);

        sound->is_used = true;
        sound->is_playing = true;
        sound->volume = volume;
        sound->pitch = pitch;
        sound->pan = pan;
        sound->is_looped = is_looped;
        sound->current_sample = 0.0f;
        sound->sample = &m_LoadedSamples[sample_id];

        M_RecalculateChannelVolumes(sound_id);

        result = sound_id;
        break;
    }
    Audio_UnlockDevice();

    if (result == AUDIO_NO_SOUND) {
        LOG_ERROR("All sample buffers are used!");
    }

    return result;
}

bool Audio_Sample_IsPlaying(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    return m_Samples[sound_id].is_playing;
}

bool Audio_Sample_Pause(int32_t sound_id)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    if (m_Samples[sound_id].is_playing) {
        Audio_LockDevice();
        m_Samples[sound_id].is_playing = false;
        Audio_UnlockDevice();
    }

    return true;
}

bool Audio_Sample_PauseAll(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        if (m_Samples[sound_id].is_used) {
            Audio_Sample_Pause(sound_id);
        }
    }

    return true;
}

bool Audio_Sample_Unpause(int32_t sound_id)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    if (!m_Samples[sound_id].is_playing) {
        Audio_LockDevice();
        m_Samples[sound_id].is_playing = true;
        Audio_UnlockDevice();
    }

    return true;
}

bool Audio_Sample_UnpauseAll(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        if (m_Samples[sound_id].is_used) {
            Audio_Sample_Unpause(sound_id);
        }
    }

    return true;
}

bool Audio_Sample_Close(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    Audio_LockDevice();
    m_Samples[sound_id].is_used = false;
    m_Samples[sound_id].is_playing = false;
    Audio_UnlockDevice();

    return true;
}

bool Audio_Sample_CloseAll(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        if (m_Samples[sound_id].is_used) {
            Audio_Sample_Close(sound_id);
        }
    }

    return true;
}

bool Audio_Sample_SetPan(int32_t sound_id, int32_t pan)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    Audio_LockDevice();
    m_Samples[sound_id].pan = pan;
    M_RecalculateChannelVolumes(sound_id);
    Audio_UnlockDevice();

    return true;
}

bool Audio_Sample_SetVolume(int32_t sound_id, int32_t volume)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    Audio_LockDevice();
    m_Samples[sound_id].volume = volume;
    M_RecalculateChannelVolumes(sound_id);
    Audio_UnlockDevice();

    return true;
}

bool Audio_Sample_SetPitch(int32_t sound_id, float pitch)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    Audio_LockDevice();
    m_Samples[sound_id].pitch = pitch;
    M_RecalculateChannelVolumes(sound_id);
    Audio_UnlockDevice();

    return true;
}

void Audio_Sample_Mix(float *dst_buffer, size_t len)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
        if (!sound->is_playing) {
            continue;
        }

        int32_t samples_requested =
            len / sizeof(AUDIO_WORKING_FORMAT) / AUDIO_WORKING_CHANNELS;
        float src_sample_idx = sound->current_sample;
        const float *src_buffer = sound->sample->sample_data;
        float *dst_ptr = dst_buffer;

        while ((dst_ptr - dst_buffer) / AUDIO_WORKING_CHANNELS
               < samples_requested) {

            float src_sample = 0.0f;
            for (int32_t i = 0; i < sound->sample->channels; i++) {
                src_sample += src_buffer
                    [(int32_t)src_sample_idx * sound->sample->channels + i];
            }
            src_sample /= (float)sound->sample->channels;

            *dst_ptr++ += src_sample * sound->volume_l;
            *dst_ptr++ += src_sample * sound->volume_r;
            src_sample_idx += sound->pitch;

            if ((int32_t)src_sample_idx >= sound->sample->num_samples) {
                if (sound->is_looped) {
                    src_sample_idx = 0.0f;
                } else {
                    break;
                }
            }
        }

        sound->current_sample = src_sample_idx;
        if (sound->current_sample >= sound->sample->num_samples
            && !sound->is_looped) {
            Audio_Sample_Close(sound_id);
        }
    }
}
