#include "audio_internal.h"

#include "debug.h"
#include "log.h"
#include "memory.h"

#include <SDL2/SDL_audio.h>
#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
    float volume_l; // sample gain multiplier
    float volume_r; // sample gain multiplier

    float pitch;
    int32_t volume; // volume specified in hundredths of decibel
    int32_t pan; // pan specified in hundredths of decibel

    // pitch shift means the same samples can be reused twice, hence float
    float current_sample;

    AUDIO_SAMPLE *sample;
} AUDIO_SAMPLE_SOUND;

typedef struct {
    const char *data;
    const char *ptr;
    int32_t size;
    int32_t remaining;
} AUDIO_AV_BUFFER;

static int32_t m_LoadedSamplesCount = 0;
static AUDIO_SAMPLE m_LoadedSamples[AUDIO_MAX_SAMPLES] = {};
static AUDIO_SAMPLE_SOUND m_Samples[AUDIO_MAX_ACTIVE_SAMPLES] = {};

static double M_DecibelToMultiplier(double db_gain);
static bool M_RecalculateChannelVolumes(int32_t sound_id);
static int32_t M_ReadAVBuffer(void *opaque, uint8_t *dst, int32_t dst_size);
static int64_t M_SeekAVBuffer(void *opaque, int64_t offset, int32_t whence);
static bool M_Convert(const int32_t sample_id);

static double M_DecibelToMultiplier(double db_gain)
{
    return pow(2.0, db_gain / 600.0);
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

static bool M_Convert(const int32_t sample_id)
{
    ASSERT(sample_id >= 0 && sample_id < m_LoadedSamplesCount);

    bool result = false;
    AUDIO_SAMPLE *const sample = &m_LoadedSamples[sample_id];

    if (sample->sample_data != nullptr) {
        return true;
    }

    const clock_t time_start = clock();
    size_t working_buffer_size = 0;
    float *working_buffer = nullptr;

    struct {
        size_t read_buffer_size;
        AVIOContext *avio_context;
        AVStream *stream;
        AVFormatContext *format_ctx;
        const AVCodec *codec;
        AVCodecContext *codec_ctx;
        AVPacket *packet;
        AVFrame *frame;
    } av = {
        .read_buffer_size = 8192,
        .avio_context = nullptr,
        .stream = nullptr,
        .format_ctx = nullptr,
        .codec = nullptr,
        .codec_ctx = nullptr,
        .packet = nullptr,
        .frame = nullptr,
    };

    struct {
        struct {
            int32_t format;
            AVChannelLayout ch_layout;
            int32_t sample_rate;
        } src, dst;
        SwrContext *ctx;
    } swr = {};

    int32_t error_code;

    unsigned char *read_buffer = av_malloc(av.read_buffer_size);
    if (!read_buffer) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    AUDIO_AV_BUFFER av_buf = {
        .data = sample->original_data,
        .ptr = sample->original_data,
        .size = sample->original_size,
        .remaining = sample->original_size,
    };

    av.avio_context = avio_alloc_context(
        read_buffer, av.read_buffer_size, 0, &av_buf, M_ReadAVBuffer, nullptr,
        M_SeekAVBuffer);

    av.format_ctx = avformat_alloc_context();
    av.format_ctx->pb = av.avio_context;
    error_code =
        avformat_open_input(&av.format_ctx, "dummy_filename", nullptr, nullptr);
    if (error_code != 0) {
        goto cleanup;
    }

    error_code = avformat_find_stream_info(av.format_ctx, nullptr);
    if (error_code < 0) {
        goto cleanup;
    }

    av.stream = nullptr;
    for (uint32_t i = 0; i < av.format_ctx->nb_streams; i++) {
        AVStream *current_stream = av.format_ctx->streams[i];
        if (current_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            av.stream = current_stream;
            break;
        }
    }
    if (!av.stream) {
        error_code = AVERROR_STREAM_NOT_FOUND;
        goto cleanup;
    }

    av.codec = avcodec_find_decoder(av.stream->codecpar->codec_id);
    if (!av.codec) {
        error_code = AVERROR_DEMUXER_NOT_FOUND;
        goto cleanup;
    }

    av.codec_ctx = avcodec_alloc_context3(av.codec);
    if (!av.codec_ctx) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    error_code =
        avcodec_parameters_to_context(av.codec_ctx, av.stream->codecpar);
    if (error_code) {
        goto cleanup;
    }

    error_code = avcodec_open2(av.codec_ctx, av.codec, nullptr);
    if (error_code < 0) {
        goto cleanup;
    }

    av.packet = av_packet_alloc();
    if (!av.packet) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    av.frame = av_frame_alloc();
    if (!av.frame) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    while (1) {
        error_code = av_read_frame(av.format_ctx, av.packet);
        if (error_code == AVERROR_EOF) {
            av_packet_unref(av.packet);
            error_code = 0;
            break;
        }

        if (error_code < 0) {
            av_packet_unref(av.packet);
            goto cleanup;
        }

        error_code = avcodec_send_packet(av.codec_ctx, av.packet);
        if (error_code < 0) {
            av_packet_unref(av.packet);
            goto cleanup;
        }

        if (swr.ctx == nullptr) {
            swr.src.sample_rate = av.codec_ctx->sample_rate;
            swr.src.ch_layout = av.codec_ctx->ch_layout;
            swr.src.format = av.codec_ctx->sample_fmt;
            swr.dst.sample_rate = AUDIO_WORKING_RATE;
            av_channel_layout_default(&swr.dst.ch_layout, 1);
            swr.dst.format = Audio_GetAVAudioFormat(AUDIO_WORKING_FORMAT);
            swr_alloc_set_opts2(
                &swr.ctx, &swr.dst.ch_layout, swr.dst.format,
                swr.dst.sample_rate, &swr.src.ch_layout, swr.src.format,
                swr.src.sample_rate, 0, 0);
            if (swr.ctx == nullptr) {
                av_packet_unref(av.packet);
                error_code = AVERROR(ENOMEM);
                goto cleanup;
            }

            error_code = swr_init(swr.ctx);
            if (error_code != 0) {
                av_packet_unref(av.packet);
                goto cleanup;
            }
        }

        while (1) {
            error_code = avcodec_receive_frame(av.codec_ctx, av.frame);
            if (error_code == AVERROR(EAGAIN)) {
                av_frame_unref(av.frame);
                break;
            }

            if (error_code < 0) {
                av_packet_unref(av.packet);
                av_frame_unref(av.frame);
                goto cleanup;
            }

            uint8_t *out_buffer = nullptr;
            const int32_t out_samples =
                swr_get_out_samples(swr.ctx, av.frame->nb_samples);
            av_samples_alloc(
                &out_buffer, nullptr, swr.dst.ch_layout.nb_channels,
                out_samples, swr.dst.format, 1);
            int32_t resampled_size = swr_convert(
                swr.ctx, &out_buffer, out_samples,
                (const uint8_t **)av.frame->data, av.frame->nb_samples);
            while (resampled_size > 0) {
                int32_t out_buffer_size = av_samples_get_buffer_size(
                    nullptr, swr.dst.ch_layout.nb_channels, resampled_size,
                    swr.dst.format, 1);

                if (out_buffer_size > 0) {
                    working_buffer = Memory_Realloc(
                        working_buffer, working_buffer_size + out_buffer_size);
                    if (out_buffer) {
                        memcpy(
                            (uint8_t *)working_buffer + working_buffer_size,
                            out_buffer, out_buffer_size);
                    }
                    working_buffer_size += out_buffer_size;
                }

                resampled_size =
                    swr_convert(swr.ctx, &out_buffer, out_samples, nullptr, 0);
            }

            av_freep(&out_buffer);
            av_frame_unref(av.frame);
        }

        av_packet_unref(av.packet);
    }

    int32_t sample_format_bytes = av_get_bytes_per_sample(swr.dst.format);
    sample->num_samples = working_buffer_size / sample_format_bytes
        / swr.dst.ch_layout.nb_channels;
    sample->channels = swr.src.ch_layout.nb_channels;
    sample->sample_data = working_buffer;
    result = true;

    const clock_t time_end = clock();
    const double time_delta =
        (((double)(time_end - time_start)) / CLOCKS_PER_SEC) * 1000.0f;
    LOG_DEBUG(
        "Sample %d decoded (%.0f ms)", sample_id, sample->original_size,
        time_delta);

cleanup:
    if (error_code != 0) {
        LOG_ERROR(
            "Error while opening sample ID %d: %s", sample_id,
            av_err2str(error_code));
    }

    if (swr.ctx) {
        swr_free(&swr.ctx);
    }

    if (av.frame) {
        av_frame_free(&av.frame);
    }

    if (av.packet) {
        av_packet_free(&av.packet);
    }

    av.codec = nullptr;

    if (!result) {
        sample->sample_data = nullptr;
        sample->original_data = nullptr;
        sample->original_size = 0;
        sample->num_samples = 0;
        sample->channels = 0;
        Memory_FreePointer(&working_buffer);
    }

    if (av.codec_ctx) {
        avcodec_free_context(&av.codec_ctx);
    }

    if (av.format_ctx) {
        avformat_close_input(&av.format_ctx);
    }

    if (av.avio_context) {
        av_freep(&av.avio_context->buffer);
        avio_context_free(&av.avio_context);
    }

    return result;
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

    bool result = false;
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

bool Audio_Sample_LoadSingle(
    const int32_t sample_id, const char *const data, const size_t size)
{
    ASSERT(data != nullptr);

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

bool Audio_Sample_LoadMany(size_t count, const char **contents, size_t *sizes)
{
    ASSERT(contents != nullptr);
    ASSERT(sizes != nullptr);

    if (!g_AudioDeviceID) {
        return false;
    }

    ASSERT(count <= AUDIO_MAX_SAMPLES);

    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();

    bool result = true;
    for (int32_t i = 0; i < (int32_t)count; i++) {
        result &= Audio_Sample_LoadSingle(i, contents[i], sizes[i]);
    }
    if (!result) {
        Audio_Sample_UnloadAll();
    }
    return result;
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

    SDL_LockAudioDevice(g_AudioDeviceID);
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES;
         sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_Samples[sound_id];
        if (sound->is_used) {
            continue;
        }

        M_Convert(sample_id);

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
    SDL_UnlockAudioDevice(g_AudioDeviceID);

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
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_Samples[sound_id].is_playing = false;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
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
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_Samples[sound_id].is_playing = true;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
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

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_Samples[sound_id].is_used = false;
    m_Samples[sound_id].is_playing = false;
    SDL_UnlockAudioDevice(g_AudioDeviceID);

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

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_Samples[sound_id].pan = pan;
    M_RecalculateChannelVolumes(sound_id);
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

bool Audio_Sample_SetVolume(int32_t sound_id, int32_t volume)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_Samples[sound_id].volume = volume;
    M_RecalculateChannelVolumes(sound_id);
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

bool Audio_Sample_SetPitch(int32_t sound_id, float pitch)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_Samples[sound_id].pitch = pitch;
    M_RecalculateChannelVolumes(sound_id);
    SDL_UnlockAudioDevice(g_AudioDeviceID);

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

            // because we handle 3d sound ourselves, downmix to mono
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
