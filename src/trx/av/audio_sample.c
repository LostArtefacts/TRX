#include <trx/av/audio_internal.h>

#include <trx/av/audio_decoder.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/version.h>

#include <SDL2/SDL_atomic.h>
#include <SDL2/SDL_audio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

// Samples are mixed in mono: the game does its own 3D panning.
#define SAMPLE_CHANNELS 1

// How much new audio a single sample may produce per sweep, so a long sample
// never holds up the streams sharing the worker.
#define SAMPLE_CHUNK_SAMPLES (AUDIO_WORKING_RATE / 4)

// Decoding writes a few samples more than the container duration suggests, as
// the resampler rounds up. Room for a second of it costs nothing.
#define SAMPLE_LENGTH_SLACK AUDIO_WORKING_RATE

typedef struct {
    AUDIO_DECODER *decoder;

    float *pcm;
    int32_t count;
    int32_t capacity;
    // Set when the container did not say how long the sample is. The buffer
    // then has to grow, so it stays private until the decode finishes.
    bool can_grow;
} AUDIO_SAMPLE_DECODER;

typedef struct {
    char *original_data;
    size_t original_size;
    int32_t channels;

    // Owned by the decoder until it publishes it. The mixer reads
    // `sample_data` only up to `ready_samples`, which the worker raises as it
    // decodes, so the buffer is never reallocated underneath the mixer.
    float *sample_data;
    SDL_atomic_t ready_samples;
    bool is_decoded;
    bool is_queued;
    AUDIO_SAMPLE_DECODER *decoder;
} AUDIO_SAMPLE;

typedef struct {
    bool is_used;
    bool is_looped;
    bool is_playing;
    float volume_l; // sample gain multiplier
    float volume_r; // sample gain multiplier
    // Follows `volume_l`/`volume_r` over one callback chunk.
    float current_volume_l;
    float current_volume_r;

    float pitch;
    // `volume`/`pan` come from the game layer (src/trx/game/sound/common.c).
    // Despite the historic "decibel" naming, these values are not base-10
    // centi-dB. They are a log2-based gain domain that the OG engine used to
    // feed directly to DirectSound (-10000..0 style range).
    //
    // In TRX we keep the game-side math pristine and interpret these as:
    //   TR1/2: log2(gain) * 1000
    //   TR3:   DirectSound-style centi-dB in [-10000..0]
    //
    // `M_DecibelToMultiplier()` is the corresponding inverse transform:
    //   TR1/2: gain = 2^(value/1000)
    //   TR3:   gain = 10^(value/2000)
    //
    // This makes combining contributions (volume + pan) an additive operation
    // in the game's log domain, while still producing a linear multiplier for
    // the mixer.
    int32_t volume;
    int32_t pan;

    // pitch shift means the same samples can be reused twice, hence float
    float current_sample;

    AUDIO_SAMPLE *sample;
} AUDIO_SAMPLE_SOUND;

static int32_t m_LoadedSamplesCount = 0;
static AUDIO_SAMPLE m_LoadedSamples[AUDIO_MAX_SAMPLES] = {};
static AUDIO_SAMPLE_SOUND m_Samples[AUDIO_MAX_ACTIVE_SAMPLES] = {};
static int32_t m_DecodeQueue[AUDIO_MAX_SAMPLES] = {};
static int32_t m_DecodeQueueCount = 0;

static RESULT M_CheckSampleID(const int32_t sample_id)
{
    FAIL_IF(
        sample_id < 0 || sample_id >= AUDIO_MAX_SAMPLES,
        "sample %d is outside the %d samples that fit", sample_id,
        AUDIO_MAX_SAMPLES);
    return OK;
}

static RESULT M_CheckSoundID(const int32_t sound_id)
{
    MUST(Audio_CheckDevice());
    FAIL_IF(
        sound_id < 0 || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES,
        "sound %d is not playing", sound_id);
    return OK;
}

static double M_DecibelToMultiplier(double db_gain)
{
    if (g_TRVersion < 3) {
        // Legacy scale
        return pow(2.0, db_gain / 600.0);
    } else {
        // DirectSound-style centi-dB domain: gain = 10^(centi_dB/2000).
        return pow(10.0, db_gain / 2000.0);
    }
}

static float M_ReadMono(const AUDIO_SAMPLE *const sample, const int32_t idx)
{
    float result = 0.0f;
    for (int32_t i = 0; i < sample->channels; i++) {
        result += sample->sample_data[idx * sample->channels + i];
    }
    return result / (float)sample->channels;
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

// Hands the decoder's own buffer over, or frees it when the sample never got
// it. Everything else the decode needed goes with it.
static void M_DecoderFree(AUDIO_SAMPLE *const sample)
{
    AUDIO_SAMPLE_DECODER *const decoder = sample->decoder;
    if (decoder == nullptr) {
        return;
    }

    if (decoder->pcm != sample->sample_data) {
        Memory_Free(decoder->pcm);
    }
    AudioDecoder_Free(&decoder->decoder);
    Memory_FreePointer(&sample->decoder);
}

static bool M_DecoderOpen(AUDIO_SAMPLE *const sample)
{
    ASSERT(sample->decoder == nullptr);

    AUDIO_DECODER *source = nullptr;
    if (!SHOULD(AudioDecoder_CreateFromMemory(
            (const uint8_t *)sample->original_data, sample->original_size,
            SAMPLE_CHANNELS, &source))) {
        return false;
    }

    AUDIO_SAMPLE_DECODER *const decoder =
        Memory_Alloc(sizeof(AUDIO_SAMPLE_DECODER));
    sample->decoder = decoder;
    decoder->decoder = source;

    const double duration = AudioDecoder_GetDuration(source);
    decoder->can_grow = duration <= 0.0;
    decoder->capacity = decoder->can_grow
        ? AUDIO_WORKING_RATE
        : (int32_t)(duration * AUDIO_WORKING_RATE) + SAMPLE_LENGTH_SLACK;
    decoder->pcm = Memory_Alloc(decoder->capacity * sizeof(float));

    sample->channels = SAMPLE_CHANNELS;
    if (!decoder->can_grow) {
        sample->sample_data = decoder->pcm;
    }
    return true;
}

static void M_DecoderAppend(
    AUDIO_SAMPLE *const sample, const float *const data, const int32_t count)
{
    AUDIO_SAMPLE_DECODER *const decoder = sample->decoder;

    if (decoder->count + count > decoder->capacity) {
        if (!decoder->can_grow) {
            LOG_ERROR("Sample runs past the length its container declares");
            return;
        }
        decoder->capacity = MAX(decoder->capacity * 2, decoder->count + count);
        decoder->pcm =
            Memory_Realloc(decoder->pcm, decoder->capacity * sizeof(float));
    }

    memcpy(decoder->pcm + decoder->count, data, count * sizeof(float));
    decoder->count += count;

    if (!decoder->can_grow) {
        SDL_AtomicSet(&sample->ready_samples, decoder->count);
    }
}

static bool M_DecoderStep(AUDIO_SAMPLE *const sample)
{
    const float *samples = nullptr;
    const int32_t count = AudioDecoder_Read(sample->decoder->decoder, &samples);
    if (count < 0) {
        return false;
    }
    if (count > 0) {
        M_DecoderAppend(sample, samples, count);
    }
    return true;
}

static void M_DecoderClose(AUDIO_SAMPLE *const sample)
{
    AUDIO_SAMPLE_DECODER *const decoder = sample->decoder;

    if (decoder->can_grow) {
        Audio_LockDevice();
        sample->sample_data = decoder->pcm;
        SDL_AtomicSet(&sample->ready_samples, decoder->count);
        Audio_UnlockDevice();
    }

    sample->is_decoded = true;
    M_DecoderFree(sample);
}

static void M_DequeueDecode(const int32_t queue_idx)
{
    m_LoadedSamples[m_DecodeQueue[queue_idx]].is_queued = false;
    m_DecodeQueue[queue_idx] = m_DecodeQueue[--m_DecodeQueueCount];
}

static void M_QueueDecode(const int32_t sample_id)
{
    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    if (sample->is_decoded || sample->is_queued) {
        return;
    }
    sample->is_queued = true;
    m_DecodeQueue[m_DecodeQueueCount++] = sample_id;
}

static void M_CancelDecode(AUDIO_SAMPLE *const sample)
{
    for (int32_t i = 0; i < m_DecodeQueueCount; i++) {
        if (&m_LoadedSamples[m_DecodeQueue[i]] == sample) {
            M_DequeueDecode(i);
            break;
        }
    }
    M_DecoderFree(sample);
}

// Drops every sound playing the given sample, so its buffer can be freed. The
// device lock must be held.
static void M_DropSoundsOfSample(const AUDIO_SAMPLE *const sample)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *const sound = &m_Samples[sound_id];
        if (sound->sample == sample) {
            sound->is_used = false;
            sound->is_playing = false;
            sound->sample = nullptr;
        }
    }
}

static void M_UnloadSample(AUDIO_SAMPLE *const sample)
{
    if (sample->original_data == nullptr && sample->sample_data == nullptr) {
        return;
    }

    M_CancelDecode(sample);

    Audio_LockDevice();
    M_DropSoundsOfSample(sample);
    Memory_FreePointer(&sample->sample_data);
    SDL_AtomicSet(&sample->ready_samples, 0);
    Audio_UnlockDevice();

    Memory_FreePointer(&sample->original_data);
    sample->original_size = 0;
    sample->channels = 0;
    sample->is_decoded = false;
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
    IGNORE(Audio_Sample_CloseAll());
    IGNORE(Audio_Sample_UnloadAll());
}

void Audio_Sample_Pump(void)
{
    Audio_WorkerLock();
    for (int32_t i = 0; i < m_DecodeQueueCount; i++) {
        AUDIO_SAMPLE *const sample = &m_LoadedSamples[m_DecodeQueue[i]];

        if (sample->decoder == nullptr && !M_DecoderOpen(sample)) {
            M_DequeueDecode(i--);
            continue;
        }

        const int32_t target = sample->decoder->count + SAMPLE_CHUNK_SAMPLES;
        while (sample->decoder->count < target) {
            if (!M_DecoderStep(sample)) {
                M_DecoderClose(sample);
                M_DequeueDecode(i--);
                break;
            }
        }
    }
    Audio_WorkerUnlock();
}

bool Audio_Sample_IsLoaded(const int32_t sample_id)
{
    if (sample_id < 0 || sample_id >= AUDIO_MAX_SAMPLES) {
        return false;
    }
    return m_LoadedSamples[sample_id].original_data != nullptr;
}

RESULT Audio_Sample_Unload(const int32_t sample_id)
{
    MUST(M_CheckSampleID(sample_id));

    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    FAIL_IF(
        sample->original_data == nullptr, "sample %d is already unloaded",
        sample_id);

    Audio_WorkerLock();
    M_UnloadSample(sample);
    Audio_WorkerUnlock();
    m_LoadedSamplesCount--;
    return OK;
}

RESULT Audio_Sample_UnloadAll(void)
{
    Audio_WorkerLock();
    m_LoadedSamplesCount = 0;
    for (int32_t i = 0; i < AUDIO_MAX_SAMPLES; i++) {
        M_UnloadSample(&m_LoadedSamples[i]);
    }
    Audio_WorkerUnlock();
    return OK;
}

RESULT Audio_Sample_Load(
    const int32_t sample_id, const char *const data, const size_t size)
{
    FAIL_IF(
        data == nullptr || size == 0, "sample %d carries no data", sample_id);
    MUST(Audio_CheckDevice());
    MUST(M_CheckSampleID(sample_id));

    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    FAIL_IF(
        sample->original_data != nullptr,
        "sample %d is already loaded, and %zu bytes would overwrite it",
        sample_id, size);

    sample->original_data = Memory_Alloc(size);
    sample->original_size = size;
    memcpy(sample->original_data, data, size);
    m_LoadedSamplesCount++;
    return OK;
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

    if (m_LoadedSamples[sample_id].original_data == nullptr) {
        return AUDIO_NO_SOUND;
    }

    int32_t result = AUDIO_NO_SOUND;

    Audio_WorkerLock();
    M_QueueDecode(sample_id);

    Audio_LockDevice();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
        if (sound->is_used) {
            continue;
        }

        sound->is_used = true;
        sound->is_playing = true;
        sound->volume = volume;
        sound->pitch = pitch;
        sound->pan = pan;
        sound->is_looped = is_looped;
        sound->current_sample = 0.0f;
        sound->sample = &m_LoadedSamples[sample_id];

        M_RecalculateChannelVolumes(sound_id);
        sound->current_volume_l = sound->volume_l;
        sound->current_volume_r = sound->volume_r;

        result = sound_id;
        break;
    }
    Audio_UnlockDevice();
    Audio_WorkerUnlock();

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

RESULT Audio_Sample_Pause(int32_t sound_id)
{
    MUST(Audio_CheckDevice());
    if (m_Samples[sound_id].is_playing) {
        Audio_LockDevice();
        m_Samples[sound_id].is_playing = false;
        Audio_UnlockDevice();
    }
    return OK;
}

RESULT Audio_Sample_PauseAll(void)
{
    MUST(Audio_CheckDevice());
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        if (m_Samples[sound_id].is_used) {
            IGNORE(Audio_Sample_Pause(sound_id));
        }
    }
    return OK;
}

RESULT Audio_Sample_Unpause(int32_t sound_id)
{
    MUST(Audio_CheckDevice());
    if (!m_Samples[sound_id].is_playing) {
        Audio_LockDevice();
        m_Samples[sound_id].is_playing = true;
        Audio_UnlockDevice();
    }
    return OK;
}

RESULT Audio_Sample_UnpauseAll(void)
{
    MUST(Audio_CheckDevice());
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        if (m_Samples[sound_id].is_used) {
            IGNORE(Audio_Sample_Unpause(sound_id));
        }
    }
    return OK;
}

RESULT Audio_Sample_Close(int32_t sound_id)
{
    MUST(M_CheckSoundID(sound_id));
    Audio_LockDevice();
    m_Samples[sound_id].is_used = false;
    m_Samples[sound_id].is_playing = false;
    Audio_UnlockDevice();
    return OK;
}

RESULT Audio_Sample_CloseAll(void)
{
    MUST(Audio_CheckDevice());
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        if (m_Samples[sound_id].is_used) {
            IGNORE(Audio_Sample_Close(sound_id));
        }
    }
    return OK;
}

RESULT Audio_Sample_SetPan(int32_t sound_id, int32_t pan)
{
    MUST(M_CheckSoundID(sound_id));

    Audio_LockDevice();
    m_Samples[sound_id].pan = pan;
    M_RecalculateChannelVolumes(sound_id);
    Audio_UnlockDevice();

    return OK;
}

RESULT Audio_Sample_SetVolume(int32_t sound_id, int32_t volume)
{
    MUST(M_CheckSoundID(sound_id));

    Audio_LockDevice();
    m_Samples[sound_id].volume = volume;
    M_RecalculateChannelVolumes(sound_id);
    Audio_UnlockDevice();

    return OK;
}

RESULT Audio_Sample_SetPitch(int32_t sound_id, float pitch)
{
    MUST(M_CheckSoundID(sound_id));

    Audio_LockDevice();
    m_Samples[sound_id].pitch = pitch;
    M_RecalculateChannelVolumes(sound_id);
    Audio_UnlockDevice();

    return OK;
}

void Audio_Sample_Mix(float *dst_buffer, size_t len)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
        if (!sound->is_playing) {
            continue;
        }

        AUDIO_SAMPLE *const sample = sound->sample;
        const int32_t ready = SDL_AtomicGet(&sample->ready_samples);
        if (ready <= 0) {
            continue;
        }

        int32_t samples_requested =
            len / sizeof(AUDIO_WORKING_FORMAT) / AUDIO_WORKING_CHANNELS;
        float src_sample_idx = sound->current_sample;
        float *dst_ptr = dst_buffer;

        float gain_l = sound->current_volume_l;
        float gain_r = sound->current_volume_r;
        const float gain_step_l =
            (sound->volume_l - gain_l) / (float)samples_requested;
        const float gain_step_r =
            (sound->volume_r - gain_r) / (float)samples_requested;

        while ((dst_ptr - dst_buffer) / AUDIO_WORKING_CHANNELS
               < samples_requested) {

            if ((int32_t)src_sample_idx >= ready) {
                // the decoder has not caught up yet, or the sample ended
                if (!sample->is_decoded || !sound->is_looped) {
                    break;
                }
                src_sample_idx -= (float)ready;
                if ((int32_t)src_sample_idx >= ready) {
                    src_sample_idx = 0.0f;
                }
            }

            const int32_t idx = (int32_t)src_sample_idx;
            int32_t next_idx = idx + 1;
            if (next_idx >= ready) {
                next_idx = sound->is_looped && sample->is_decoded ? 0 : idx;
            }
            const float frac = src_sample_idx - (float)idx;
            const float src_sample = M_ReadMono(sample, idx) * (1.0f - frac)
                + M_ReadMono(sample, next_idx) * frac;

            *dst_ptr++ += src_sample * gain_l;
            *dst_ptr++ += src_sample * gain_r;
            gain_l += gain_step_l;
            gain_r += gain_step_r;
            src_sample_idx += sound->pitch;
        }

        sound->current_volume_l = gain_l;
        sound->current_volume_r = gain_r;
        sound->current_sample = src_sample_idx;
        if (sample->is_decoded && !sound->is_looped
            && (int32_t)src_sample_idx >= ready) {
            IGNORE(Audio_Sample_Close(sound_id));
        }
    }
}
