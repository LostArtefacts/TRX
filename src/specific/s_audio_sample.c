#define S_AUDIO_IMPL
#include "specific/s_audio.h"

#include "game/shell.h"
#include "log.h"
#include "shared/memory.h"

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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct AUDIO_SAMPLE {
    float *sample_data;
    int channels;
    int num_samples;
} AUDIO_SAMPLE;

typedef struct AUDIO_SAMPLE_SOUND {
    bool is_used;
    bool is_looped;
    bool is_playing;
    float volume_l; // sample gain multiplier
    float volume_r; // sample gain multiplier

    float pitch;
    int volume; // volume specified in hundredths of decibel
    int pan; // pan specified in hundredths of decibel

    // pitch shift means the same samples can be reused twice, hence float
    float current_sample;

    AUDIO_SAMPLE *sample;
} AUDIO_SAMPLE_SOUND;

typedef struct AUDIO_AV_BUFFER {
    const char *data;
    const char *ptr;
    int size;
    int remaining;
} AUDIO_AV_BUFFER;

static int m_LoadedSamplesCount = 0;
static AUDIO_SAMPLE m_LoadedSamples[AUDIO_MAX_SAMPLES] = { 0 };
static AUDIO_SAMPLE_SOUND m_SampleSounds[AUDIO_MAX_ACTIVE_SAMPLES] = { 0 };

static double S_Audio_DecibelToMultiplier(double db_gain)
{
    return pow(2.0, db_gain / 600.0);
}

static bool S_Audio_SampleRecalculateChannelVolumes(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    AUDIO_SAMPLE_SOUND *sound = &m_SampleSounds[sound_id];
    sound->volume_l = S_Audio_DecibelToMultiplier(
        sound->volume - (sound->pan > 0 ? sound->pan : 0));
    sound->volume_r = S_Audio_DecibelToMultiplier(
        sound->volume + (sound->pan < 0 ? sound->pan : 0));

    return true;
}

static int S_Audio_ReadAVBuffer(void *opaque, uint8_t *dst, int dst_size)
{
    AUDIO_AV_BUFFER *src = opaque;
    int read = dst_size >= src->remaining ? src->remaining : dst_size;
    if (!read) {
        return AVERROR_EOF;
    }
    memcpy(dst, src->ptr, read);
    src->ptr += read;
    src->remaining -= read;
    return read;
}

static int64_t S_Audio_SeekAVBuffer(void *opaque, int64_t offset, int whence)
{
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

static bool S_Audio_SampleLoad(int sample_id, const char *content, size_t size)
{
    if (!g_AudioDeviceID || sample_id < 0 || sample_id >= AUDIO_MAX_SAMPLES) {
        return false;
    }

    bool ret = false;
    AUDIO_SAMPLE *sample = &m_LoadedSamples[sample_id];

    size_t working_buffer_size = 0;
    float *working_buffer = NULL;

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
        .avio_context = NULL,
        .stream = NULL,
        .format_ctx = NULL,
        .codec = NULL,
        .codec_ctx = NULL,
        .packet = NULL,
        .frame = NULL,
    };

    struct {
        int src_format;
        int src_channels;
        int src_sample_rate;
        int dst_format;
        int dst_channels;
        int dst_sample_rate;
        SwrContext *ctx;
    } swr = { 0 };

    int error_code;

    unsigned char *read_buffer = av_malloc(av.read_buffer_size);
    if (!read_buffer) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    AUDIO_AV_BUFFER av_buf = {
        .data = content,
        .ptr = content,
        .size = size,
        .remaining = size,
    };

    av.avio_context = avio_alloc_context(
        read_buffer, av.read_buffer_size, 0, &av_buf, S_Audio_ReadAVBuffer,
        NULL, S_Audio_SeekAVBuffer);

    av.format_ctx = avformat_alloc_context();
    av.format_ctx->pb = av.avio_context;
    error_code =
        avformat_open_input(&av.format_ctx, "dummy_filename", NULL, NULL);
    if (error_code != 0) {
        goto cleanup;
    }

    error_code = avformat_find_stream_info(av.format_ctx, NULL);
    if (error_code < 0) {
        goto cleanup;
    }

    av.stream = NULL;
    for (unsigned int i = 0; i < av.format_ctx->nb_streams; i++) {
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

    error_code = avcodec_open2(av.codec_ctx, av.codec, NULL);
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

        if (!swr.ctx) {
            swr.src_sample_rate = av.codec_ctx->sample_rate;
            swr.src_channels = av.codec_ctx->channels;
            swr.src_format = av.codec_ctx->sample_fmt;
            swr.dst_sample_rate = AUDIO_WORKING_RATE;
            swr.dst_channels = 1;
            swr.dst_format = S_Audio_GetAVAudioFormat(AUDIO_WORKING_FORMAT);
            swr.ctx = swr_alloc_set_opts(
                swr.ctx, swr.dst_channels, swr.dst_format, swr.dst_sample_rate,
                swr.src_channels, swr.src_format, swr.src_sample_rate, 0, 0);
            if (!swr.ctx) {
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

            uint8_t *out_buffer = NULL;
            const int out_samples =
                swr_get_out_samples(swr.ctx, av.frame->nb_samples);
            av_samples_alloc(
                &out_buffer, NULL, swr.dst_channels, out_samples,
                swr.dst_format, 1);
            int resampled_size = swr_convert(
                swr.ctx, &out_buffer, out_samples,
                (const uint8_t **)av.frame->data, av.frame->nb_samples);
            while (resampled_size > 0) {
                int out_buffer_size = av_samples_get_buffer_size(
                    NULL, swr.dst_channels, resampled_size, swr.dst_format, 1);

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
                    swr_convert(swr.ctx, &out_buffer, out_samples, NULL, 0);
            }

            av_freep(&out_buffer);
            av_frame_unref(av.frame);
        }

        av_packet_unref(av.packet);
    }

    int sample_format_bytes = av_get_bytes_per_sample(swr.dst_format);
    sample->num_samples =
        working_buffer_size / sample_format_bytes / swr.dst_channels;
    sample->channels = swr.src_channels;
    sample->sample_data = working_buffer;

    ret = true;

cleanup:
    if (error_code > 0) {
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

    av.codec = NULL;

    if (!ret) {
        sample->sample_data = NULL;
        sample->num_samples = 0;
        sample->channels = 0;

        Memory_FreePointer(&working_buffer);
    }

    if (av.codec_ctx) {
        avcodec_close(av.codec_ctx);
        av_freep(&av.codec_ctx);
    }

    if (av.format_ctx) {
        avformat_close_input(&av.format_ctx);
    }

    if (av.avio_context) {
        av_freep(&av.avio_context->buffer);
        avio_context_free(&av.avio_context);
    }

    return ret;
}

void S_Audio_SampleSoundInit(void)
{
    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES; sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_SampleSounds[sound_id];
        sound->is_used = false;
        sound->is_playing = false;
        sound->volume = 0.0f;
        sound->pitch = 1.0f;
        sound->pan = 0.0f;
        sound->current_sample = 0.0f;
        sound->sample = NULL;
    }
}

void S_Audio_SampleSoundShutdown(void)
{
    if (!g_AudioDeviceID) {
        return;
    }

    S_Audio_SamplesClear();
}

bool S_Audio_SamplesClear(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    S_Audio_SampleSoundCloseAll();

    for (int i = 0; i < AUDIO_MAX_SAMPLES; i++) {
        Memory_FreePointer(&m_LoadedSamples[i].sample_data);
    }

    return true;
}

bool S_Audio_SamplesLoad(size_t count, const char **contents, size_t *sizes)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    if (count > AUDIO_MAX_SAMPLES) {
        Shell_ExitSystemFmt(
            "Trying to load too many samples (maximum supported count: %d)",
            AUDIO_MAX_SAMPLES);
        return false;
    }

    S_Audio_SamplesClear();

    bool result = true;
    for (int sample_id = 0; sample_id < (int)count; sample_id++) {
        result &= S_Audio_SampleLoad(
            sample_id, contents[sample_id], sizes[sample_id]);
    }
    if (result) {
        m_LoadedSamplesCount = count;
    } else {
        S_Audio_SamplesClear();
    }
    return result;
}

int S_Audio_SampleSoundPlay(
    int sample_id, int volume, float pitch, int pan, bool is_looped)
{
    if (!g_AudioDeviceID || sample_id < 0
        || sample_id >= m_LoadedSamplesCount) {
        return AUDIO_NO_SOUND;
    }

    int result = AUDIO_NO_SOUND;

    SDL_LockAudioDevice(g_AudioDeviceID);
    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES; sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_SampleSounds[sound_id];
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

        S_Audio_SampleRecalculateChannelVolumes(sound_id);

        result = sound_id;
        break;
    }
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    if (result == AUDIO_NO_SOUND) {
        LOG_ERROR("All sample buffers are used!");
    }

    return result;
}

bool S_Audio_SampleSoundIsPlaying(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    return m_SampleSounds[sound_id].is_playing;
}

bool S_Audio_SampleSoundPause(int sound_id)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    if (m_SampleSounds[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_SampleSounds[sound_id].is_playing = false;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return true;
}

bool S_Audio_SampleSoundPauseAll(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES; sound_id++) {
        if (m_SampleSounds[sound_id].is_used) {
            S_Audio_SampleSoundPause(sound_id);
        }
    }

    return true;
}

bool S_Audio_SampleSoundUnpause(int sound_id)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    if (!m_SampleSounds[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_SampleSounds[sound_id].is_playing = true;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return true;
}

bool S_Audio_SampleSoundUnpauseAll(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES; sound_id++) {
        if (m_SampleSounds[sound_id].is_used) {
            S_Audio_SampleSoundUnpause(sound_id);
        }
    }

    return true;
}

bool S_Audio_SampleSoundClose(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_SampleSounds[sound_id].is_used = false;
    m_SampleSounds[sound_id].is_playing = false;
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

bool S_Audio_SampleSoundCloseAll(void)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES; sound_id++) {
        if (m_SampleSounds[sound_id].is_used) {
            S_Audio_SampleSoundClose(sound_id);
        }
    }

    return true;
}

bool S_Audio_SampleSoundSetPan(int sound_id, int pan)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_SampleSounds[sound_id].pan = pan;
    S_Audio_SampleRecalculateChannelVolumes(sound_id);
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

bool S_Audio_SampleSoundSetVolume(int sound_id, int volume)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_SAMPLES) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_SampleSounds[sound_id].volume = volume;
    S_Audio_SampleRecalculateChannelVolumes(sound_id);
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

void S_Audio_SampleSoundMix(float *dst_buffer, size_t len)
{
    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_SAMPLES; sound_id++) {
        AUDIO_SAMPLE_SOUND *sound = &m_SampleSounds[sound_id];
        if (!sound->is_playing) {
            continue;
        }

        int samples_requested =
            len / sizeof(AUDIO_WORKING_FORMAT) / AUDIO_WORKING_CHANNELS;
        float src_sample_idx = sound->current_sample;
        const float *src_buffer = sound->sample->sample_data;
        float *dst_ptr = dst_buffer;

        while ((dst_ptr - dst_buffer) / AUDIO_WORKING_CHANNELS
               < samples_requested) {

            // because we handle 3d sound ourselves, downmix to mono
            float src_sample = 0.0f;
            for (int i = 0; i < sound->sample->channels; i++) {
                src_sample += src_buffer
                    [(int)src_sample_idx * sound->sample->channels + i];
            }
            src_sample /= (float)sound->sample->channels;

            *dst_ptr++ += src_sample * sound->volume_l;
            *dst_ptr++ += src_sample * sound->volume_r;
            src_sample_idx += sound->pitch;

            if ((int)src_sample_idx >= sound->sample->num_samples) {
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
            S_Audio_SampleSoundClose(sound_id);
        }
    }
}
