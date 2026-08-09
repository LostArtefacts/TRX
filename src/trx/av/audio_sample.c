#include <trx/av/audio_internal.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/version.h>

#include <SDL2/SDL_atomic.h>
#include <SDL2/SDL_audio.h>
#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AVIO_BUFFER_SIZE 8192

// Samples are mixed in mono: the game does its own 3D panning.
#define SAMPLE_CHANNELS 1

// How much new audio a single sample may produce per sweep, so a long sample
// never holds up the streams sharing the worker.
#define SAMPLE_CHUNK_SAMPLES (AUDIO_WORKING_RATE / 4)

// Decoding writes a few samples more than the container duration suggests, as
// the resampler rounds up. Room for a second of it costs nothing.
#define SAMPLE_LENGTH_SLACK AUDIO_WORKING_RATE

typedef struct {
    const uint8_t *data;
    const uint8_t *ptr;
    int32_t size;
    int32_t remaining;
} AUDIO_AV_BUFFER;

typedef struct {
    AUDIO_AV_BUFFER buffer;
    AVIOContext *avio_ctx;
    AVFormatContext *format_ctx;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    AVPacket *packet;
    AVFrame *frame;
    SwrContext *swr_ctx;

    float *pcm;
    int32_t count;
    int32_t capacity;
    // Set when the container did not say how long the sample is. The buffer
    // then has to grow, so it stays private until the decode finishes.
    bool can_grow;

    float *convert_buffer;
    int32_t convert_capacity;
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

static int32_t M_ReadAVBuffer(void *opaque, uint8_t *dst, int32_t dst_size)
{
    ASSERT(opaque != nullptr);
    ASSERT(dst != nullptr);
    AUDIO_AV_BUFFER *src = opaque;
    int32_t read = dst_size >= src->remaining ? src->remaining : dst_size;
    if (!read) {
        return AVERROR_EOF;
    }
    memcpy(dst, src->ptr, read);
    src->ptr += read;
    src->remaining -= read;
    return read;
}

static int64_t M_SeekAVBuffer(void *opaque, int64_t offset, int32_t whence)
{
    ASSERT(opaque != nullptr);
    AUDIO_AV_BUFFER *src = opaque;
    if (whence & AVSEEK_SIZE) {
        return src->size;
    }
    switch (whence) {
    case SEEK_SET:
        if (src->size - offset < 0) {
            return AVERROR_EOF;
        }
        src->ptr = src->data + offset;
        src->remaining = src->size - offset;
        break;
    case SEEK_CUR:
        if (src->remaining - offset < 0) {
            return AVERROR_EOF;
        }
        src->ptr += offset;
        src->remaining -= offset;
        break;
    case SEEK_END:
        if (src->size + offset < 0) {
            return AVERROR_EOF;
        }
        src->ptr = src->data - offset;
        src->remaining = src->size + offset;
        break;
    }
    return src->ptr - src->data;
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
    Memory_FreePointer(&decoder->convert_buffer);

    if (decoder->swr_ctx != nullptr) {
        swr_free(&decoder->swr_ctx);
    }
    if (decoder->frame != nullptr) {
        av_frame_free(&decoder->frame);
    }
    if (decoder->packet != nullptr) {
        av_packet_free(&decoder->packet);
    }
    if (decoder->codec_ctx != nullptr) {
        avcodec_free_context(&decoder->codec_ctx);
    }
    if (decoder->format_ctx != nullptr) {
        avformat_close_input(&decoder->format_ctx);
    }
    if (decoder->avio_ctx != nullptr) {
        av_freep(&decoder->avio_ctx->buffer);
        avio_context_free(&decoder->avio_ctx);
    }

    Memory_FreePointer(&sample->decoder);
}

static bool M_DecoderOpen(AUDIO_SAMPLE *const sample)
{
    ASSERT(sample->decoder == nullptr);

    AUDIO_SAMPLE_DECODER *const decoder =
        Memory_Alloc(sizeof(AUDIO_SAMPLE_DECODER));
    sample->decoder = decoder;

    decoder->buffer = (AUDIO_AV_BUFFER) {
        .data = (const uint8_t *)sample->original_data,
        .ptr = (const uint8_t *)sample->original_data,
        .size = sample->original_size,
        .remaining = sample->original_size,
    };

    int32_t error_code = 0;
    decoder->avio_ctx = avio_alloc_context(
        av_malloc(AVIO_BUFFER_SIZE), AVIO_BUFFER_SIZE, 0, &decoder->buffer,
        M_ReadAVBuffer, nullptr, M_SeekAVBuffer);
    if (decoder->avio_ctx == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    decoder->format_ctx = avformat_alloc_context();
    if (decoder->format_ctx == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }
    decoder->format_ctx->pb = decoder->avio_ctx;

    error_code =
        avformat_open_input(&decoder->format_ctx, "mem:", nullptr, nullptr);
    if (error_code != 0) {
        goto fail;
    }

    error_code = avformat_find_stream_info(decoder->format_ctx, nullptr);
    if (error_code < 0) {
        goto fail;
    }

    for (uint32_t i = 0; i < decoder->format_ctx->nb_streams; i++) {
        AVStream *const stream = decoder->format_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            decoder->stream = stream;
            break;
        }
    }
    if (decoder->stream == nullptr) {
        error_code = AVERROR_STREAM_NOT_FOUND;
        goto fail;
    }

    const AVCodec *const codec =
        avcodec_find_decoder(decoder->stream->codecpar->codec_id);
    if (codec == nullptr) {
        error_code = AVERROR_DEMUXER_NOT_FOUND;
        goto fail;
    }

    decoder->codec_ctx = avcodec_alloc_context3(codec);
    if (decoder->codec_ctx == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    error_code = avcodec_parameters_to_context(
        decoder->codec_ctx, decoder->stream->codecpar);
    if (error_code != 0) {
        goto fail;
    }

    error_code = avcodec_open2(decoder->codec_ctx, codec, nullptr);
    if (error_code < 0) {
        goto fail;
    }

    decoder->packet = av_packet_alloc();
    decoder->frame = av_frame_alloc();
    if (decoder->packet == nullptr || decoder->frame == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    AVChannelLayout dst_layout;
    av_channel_layout_default(&dst_layout, SAMPLE_CHANNELS);
    swr_alloc_set_opts2(
        &decoder->swr_ctx, &dst_layout,
        Audio_GetAVAudioFormat(AUDIO_WORKING_FORMAT), AUDIO_WORKING_RATE,
        &decoder->codec_ctx->ch_layout, decoder->codec_ctx->sample_fmt,
        decoder->codec_ctx->sample_rate, 0, 0);
    if (decoder->swr_ctx == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }
    error_code = swr_init(decoder->swr_ctx);
    if (error_code != 0) {
        goto fail;
    }

    const int64_t duration = decoder->format_ctx->duration;
    decoder->can_grow = duration <= 0;
    decoder->capacity = decoder->can_grow
        ? AUDIO_WORKING_RATE
        : av_rescale_rnd(
              duration, AUDIO_WORKING_RATE, AV_TIME_BASE, AV_ROUND_UP)
            + SAMPLE_LENGTH_SLACK;
    decoder->pcm = Memory_Alloc(decoder->capacity * sizeof(float));

    sample->channels = SAMPLE_CHANNELS;
    if (!decoder->can_grow) {
        sample->sample_data = decoder->pcm;
    }
    return true;

fail:
    LOG_ERROR("Error while decoding sample: %s", av_err2str(error_code));
    M_DecoderFree(sample);
    return false;
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

static void M_DecoderConvert(AUDIO_SAMPLE *const sample, AVFrame *const frame)
{
    AUDIO_SAMPLE_DECODER *const decoder = sample->decoder;

    const int32_t max_samples =
        swr_get_out_samples(decoder->swr_ctx, frame->nb_samples);
    if (max_samples <= 0) {
        return;
    }

    if (max_samples > decoder->convert_capacity) {
        decoder->convert_capacity = max_samples;
        decoder->convert_buffer = Memory_Realloc(
            decoder->convert_buffer, max_samples * sizeof(float));
    }

    uint8_t *out_buffer = (uint8_t *)decoder->convert_buffer;
    const int32_t converted = swr_convert(
        decoder->swr_ctx, &out_buffer, max_samples,
        (const uint8_t **)frame->data, frame->nb_samples);
    if (converted > 0) {
        M_DecoderAppend(sample, decoder->convert_buffer, converted);
    }
}

static void M_DecoderDrain(AUDIO_SAMPLE *const sample)
{
    AUDIO_SAMPLE_DECODER *const decoder = sample->decoder;

    while (avcodec_receive_frame(decoder->codec_ctx, decoder->frame) >= 0) {
        M_DecoderConvert(sample, decoder->frame);
        av_frame_unref(decoder->frame);
    }
    av_frame_unref(decoder->frame);
}

static bool M_DecoderStep(AUDIO_SAMPLE *const sample)
{
    AUDIO_SAMPLE_DECODER *const decoder = sample->decoder;

    if (av_read_frame(decoder->format_ctx, decoder->packet) < 0) {
        avcodec_send_packet(decoder->codec_ctx, nullptr);
        M_DecoderDrain(sample);
        return false;
    }

    if (decoder->packet->stream_index == decoder->stream->index
        && avcodec_send_packet(decoder->codec_ctx, decoder->packet) >= 0) {
        M_DecoderDrain(sample);
    }
    av_packet_unref(decoder->packet);
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
    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();
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

bool Audio_Sample_Unload(const int32_t sample_id)
{
    if (sample_id < 0 || sample_id >= AUDIO_MAX_SAMPLES) {
        LOG_ERROR("Maximum allowed samples: %d", AUDIO_MAX_SAMPLES);
        return false;
    }

    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];
    if (sample->original_data == nullptr) {
        LOG_ERROR("Sample %d is already unloaded", sample_id);
        return false;
    }

    Audio_WorkerLock();
    M_UnloadSample(sample);
    Audio_WorkerUnlock();
    m_LoadedSamplesCount--;
    return true;
}

bool Audio_Sample_UnloadAll(void)
{
    Audio_WorkerLock();
    m_LoadedSamplesCount = 0;
    for (int32_t i = 0; i < AUDIO_MAX_SAMPLES; i++) {
        M_UnloadSample(&m_LoadedSamples[i]);
    }
    Audio_WorkerUnlock();
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

        AUDIO_SAMPLE *const sample = sound->sample;
        const int32_t ready = SDL_AtomicGet(&sample->ready_samples);
        if (ready <= 0) {
            continue;
        }

        int32_t samples_requested =
            len / sizeof(AUDIO_WORKING_FORMAT) / AUDIO_WORKING_CHANNELS;
        float src_sample_idx = sound->current_sample;
        const float *src_buffer = sample->sample_data;
        float *dst_ptr = dst_buffer;

        while ((dst_ptr - dst_buffer) / AUDIO_WORKING_CHANNELS
               < samples_requested) {

            if ((int32_t)src_sample_idx >= ready) {
                // the decoder has not caught up yet, or the sample ended
                if (!sample->is_decoded || !sound->is_looped) {
                    break;
                }
                src_sample_idx = 0.0f;
            }

            // because we handle 3d sound ourselves, downmix to mono
            float src_sample = 0.0f;
            for (int32_t i = 0; i < sample->channels; i++) {
                src_sample +=
                    src_buffer[(int32_t)src_sample_idx * sample->channels + i];
            }
            src_sample /= (float)sample->channels;

            *dst_ptr++ += src_sample * sound->volume_l;
            *dst_ptr++ += src_sample * sound->volume_r;
            src_sample_idx += sound->pitch;
        }

        sound->current_sample = src_sample_idx;
        if (sample->is_decoded && !sound->is_looped
            && (int32_t)src_sample_idx >= ready) {
            Audio_Sample_Close(sound_id);
        }
    }
}
