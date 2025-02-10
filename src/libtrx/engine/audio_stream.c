#include "audio_internal.h"

#include "debug.h"
#include "filesystem.h"
#include "log.h"
#include "memory.h"

#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
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
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define READ_BUFFER_SIZE                                                       \
    (AUDIO_SAMPLES * AUDIO_WORKING_CHANNELS * sizeof(AUDIO_WORKING_FORMAT))

typedef struct {
    bool is_used;
    bool is_playing;
    bool is_read_done;
    bool is_looped;
    float volume;
    double duration;
    double timestamp;

    double start_at;
    double stop_at;

    void (*finish_callback)(int32_t sound_id, void *user_data);
    void *finish_callback_user_data;

    struct {
        AVStream *stream;
        AVFormatContext *format_ctx;
        const AVCodec *codec;
        AVCodecContext *codec_ctx;
        AVPacket *packet;
        AVFrame *frame;
    } av;

    struct {
        struct {
            int32_t format;
            AVChannelLayout ch_layout;
            int32_t sample_rate;
        } src, dst;
        SwrContext *ctx;
    } swr;

    struct {
        SDL_AudioStream *stream;
    } sdl;
} AUDIO_STREAM_SOUND;

extern SDL_AudioDeviceID g_AudioDeviceID;

static AUDIO_STREAM_SOUND m_Streams[AUDIO_MAX_ACTIVE_STREAMS] = {};
static float m_MixBuffer[AUDIO_SAMPLES * AUDIO_WORKING_CHANNELS] = {};

static size_t m_DecodeBufferCapacity = 0;
static float *m_DecodeBuffer = nullptr;

static void M_SeekToStart(AUDIO_STREAM_SOUND *stream);
static bool M_DecodeFrame(AUDIO_STREAM_SOUND *stream);
static bool M_EnqueueFrame(AUDIO_STREAM_SOUND *stream);
static bool M_InitialiseFromPath(int32_t sound_id, const char *file_path);
static void M_Clear(AUDIO_STREAM_SOUND *stream);

static void M_SeekToStart(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    stream->timestamp = stream->start_at;
    if (stream->start_at <= 0.0) {
        // reset to start of file
        avio_seek(stream->av.format_ctx->pb, 0, SEEK_SET);
        avformat_seek_file(
            stream->av.format_ctx, -1, 0, 0, 0, AVSEEK_FLAG_FRAME);
    } else {
        // seek to specific timestamp
        const double time_base_sec = av_q2d(stream->av.stream->time_base);
        av_seek_frame(
            stream->av.format_ctx, 0, stream->start_at / time_base_sec,
            AVSEEK_FLAG_ANY);
    }
}

static bool M_DecodeFrame(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    if (stream->stop_at > 0.0 && stream->timestamp >= stream->stop_at) {
        if (stream->is_looped) {
            M_SeekToStart(stream);
            return M_DecodeFrame(stream);
        } else {
            return false;
        }
    }

    int32_t error_code =
        av_read_frame(stream->av.format_ctx, stream->av.packet);

    if (error_code == AVERROR_EOF && stream->is_looped) {
        M_SeekToStart(stream);
        return M_DecodeFrame(stream);
    }

    if (error_code < 0) {
        LOG_ERROR("error while decoding audio stream: %d", error_code);
        return false;
    }

    if (stream->av.packet->stream_index != stream->av.stream->index) {
        return true;
    }

    error_code = avcodec_send_packet(stream->av.codec_ctx, stream->av.packet);
    if (error_code < 0) {
        av_packet_unref(stream->av.packet);
        LOG_ERROR(
            "Got an error when decoding frame: %s", av_err2str(error_code));
        return false;
    }

    return true;
}

static bool M_EnqueueFrame(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    int32_t error_code;

    if (!stream->swr.ctx) {
        stream->swr.src.sample_rate = stream->av.codec_ctx->sample_rate;
        stream->swr.src.ch_layout = stream->av.codec_ctx->ch_layout;
        stream->swr.src.format = stream->av.codec_ctx->sample_fmt;
        stream->swr.dst.sample_rate = AUDIO_WORKING_RATE;
        av_channel_layout_default(
            &stream->swr.dst.ch_layout, AUDIO_WORKING_CHANNELS);
        stream->swr.dst.format = Audio_GetAVAudioFormat(AUDIO_WORKING_FORMAT);
        swr_alloc_set_opts2(
            &stream->swr.ctx, &stream->swr.dst.ch_layout,
            stream->swr.dst.format, stream->swr.dst.sample_rate,
            &stream->swr.src.ch_layout, stream->swr.src.format,
            stream->swr.src.sample_rate, 0, 0);
        if (!stream->swr.ctx) {
            av_packet_unref(stream->av.packet);
            error_code = AVERROR(ENOMEM);
            goto cleanup;
        }

        error_code = swr_init(stream->swr.ctx);
        if (error_code != 0) {
            av_packet_unref(stream->av.packet);
            goto cleanup;
        }
    }

    while (1) {
        error_code =
            avcodec_receive_frame(stream->av.codec_ctx, stream->av.frame);
        if (error_code == AVERROR(EAGAIN)) {
            av_frame_unref(stream->av.frame);
            error_code = 0;
            break;
        }

        if (error_code < 0) {
            av_frame_unref(stream->av.frame);
            break;
        }

        uint8_t *out_buffer = nullptr;
        const int32_t out_samples =
            swr_get_out_samples(stream->swr.ctx, stream->av.frame->nb_samples);
        av_samples_alloc(
            &out_buffer, nullptr, stream->swr.dst.ch_layout.nb_channels,
            out_samples, stream->swr.dst.format, 1);
        int32_t resampled_size = swr_convert(
            stream->swr.ctx, &out_buffer, out_samples,
            (const uint8_t **)stream->av.frame->data,
            stream->av.frame->nb_samples);

        size_t out_pos = 0;
        while (resampled_size > 0) {
            const size_t out_buffer_size = av_samples_get_buffer_size(
                nullptr, stream->swr.dst.ch_layout.nb_channels, resampled_size,
                stream->swr.dst.format, 1);

            if (out_pos + out_buffer_size > m_DecodeBufferCapacity) {
                m_DecodeBufferCapacity = out_pos + out_buffer_size;
                m_DecodeBuffer =
                    Memory_Realloc(m_DecodeBuffer, m_DecodeBufferCapacity);
            }
            if (m_DecodeBuffer != nullptr && out_buffer != nullptr) {
                memcpy(
                    (uint8_t *)m_DecodeBuffer + out_pos, out_buffer,
                    out_buffer_size);
            }
            out_pos += out_buffer_size;

            resampled_size = swr_convert(
                stream->swr.ctx, &out_buffer, out_samples, nullptr, 0);
        }

        if (SDL_AudioStreamPut(stream->sdl.stream, m_DecodeBuffer, out_pos)) {
            LOG_ERROR("Got an error when decoding frame: %s", SDL_GetError());
            av_frame_unref(stream->av.frame);
            break;
        }

        double time_base_sec = av_q2d(stream->av.stream->time_base);
        stream->timestamp =
            stream->av.frame->best_effort_timestamp * time_base_sec;
        av_freep(&out_buffer);
        av_frame_unref(stream->av.frame);
    }

    av_packet_unref(stream->av.packet);

cleanup:
    if (error_code > 0) {
        LOG_ERROR(
            "Got an error when decoding frame: %d, %s", error_code,
            av_err2str(error_code));
    }

    return true;
}

static bool M_InitialiseFromPath(int32_t sound_id, const char *file_path)
{
    ASSERT(file_path != nullptr);

    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    bool ret = false;
    SDL_LockAudioDevice(g_AudioDeviceID);

    int32_t error_code;
    char *full_path = File_GetFullPath(file_path);

    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];

    error_code = avformat_open_input(
        &stream->av.format_ctx, full_path, nullptr, nullptr);
    if (error_code != 0) {
        goto cleanup;
    }

    error_code = avformat_find_stream_info(stream->av.format_ctx, nullptr);
    if (error_code < 0) {
        goto cleanup;
    }

    stream->av.stream = nullptr;
    for (uint32_t i = 0; i < stream->av.format_ctx->nb_streams; i++) {
        AVStream *current_stream = stream->av.format_ctx->streams[i];
        if (current_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream->av.stream = current_stream;
            break;
        }
    }
    if (!stream->av.stream) {
        error_code = AVERROR_STREAM_NOT_FOUND;
        goto cleanup;
    }

    stream->av.codec =
        avcodec_find_decoder(stream->av.stream->codecpar->codec_id);
    if (!stream->av.codec) {
        error_code = AVERROR_DEMUXER_NOT_FOUND;
        goto cleanup;
    }

    stream->av.codec_ctx = avcodec_alloc_context3(stream->av.codec);
    if (!stream->av.codec_ctx) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    error_code = avcodec_parameters_to_context(
        stream->av.codec_ctx, stream->av.stream->codecpar);
    if (error_code) {
        goto cleanup;
    }

    error_code = avcodec_open2(stream->av.codec_ctx, stream->av.codec, nullptr);
    if (error_code < 0) {
        goto cleanup;
    }

    stream->av.packet = av_packet_alloc();
    if (!stream->av.packet) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    stream->av.frame = av_frame_alloc();
    if (!stream->av.frame) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    M_DecodeFrame(stream);

    const int32_t sdl_sample_rate = stream->av.codec_ctx->sample_rate;
    const int32_t sdl_channels = stream->av.codec_ctx->ch_layout.nb_channels;

    stream->is_read_done = false;
    stream->is_used = true;
    stream->is_playing = true;
    stream->is_looped = false;
    stream->volume = 1.0f;
    stream->timestamp = 0.0;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;
    stream->duration =
        (double)stream->av.format_ctx->duration / (double)AV_TIME_BASE;
    stream->start_at = -1.0; // negative value means unset
    stream->stop_at = -1.0; // negative value means unset

    stream->sdl.stream = SDL_NewAudioStream(
        AUDIO_WORKING_FORMAT, sdl_channels, AUDIO_WORKING_RATE,
        AUDIO_WORKING_FORMAT, sdl_channels, AUDIO_WORKING_RATE);
    if (!stream->sdl.stream) {
        LOG_ERROR("Failed to create SDL stream: %s", SDL_GetError());
        goto cleanup;
    }

    ret = true;
    M_EnqueueFrame(stream);

cleanup:
    if (error_code) {
        LOG_ERROR(
            "Error while opening audio %s: %s", file_path,
            av_err2str(error_code));
    }

    if (!ret) {
        Audio_Stream_Close(sound_id);
    }

    SDL_UnlockAudioDevice(g_AudioDeviceID);
    Memory_FreePointer(&full_path);
    return ret;
}

static void M_Clear(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    stream->is_used = false;
    stream->is_playing = false;
    stream->is_read_done = true;
    stream->is_looped = false;
    stream->volume = 0.0f;
    stream->duration = 0.0;
    stream->timestamp = 0.0;
    stream->sdl.stream = nullptr;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;
}

void Audio_Stream_Init(void)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        M_Clear(&m_Streams[sound_id]);
    }
}

void Audio_Stream_Shutdown(void)
{
    Memory_FreePointer(&m_DecodeBuffer);
    m_DecodeBufferCapacity = 0;
    if (!g_AudioDeviceID) {
        return;
    }

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        if (m_Streams[sound_id].is_used) {
            Audio_Stream_Close(sound_id);
        }
    }
}

bool Audio_Stream_Pause(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    if (m_Streams[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_Streams[sound_id].is_playing = false;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return true;
}

bool Audio_Stream_Unpause(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    if (!m_Streams[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_Streams[sound_id].is_playing = true;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return true;
}

int32_t Audio_Stream_CreateFromFile(const char *file_path)
{
    if (!g_AudioDeviceID) {
        return AUDIO_NO_SOUND;
    }

    ASSERT(file_path != nullptr);

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];
        if (stream->is_used) {
            continue;
        }

        if (!M_InitialiseFromPath(sound_id, file_path)) {
            return AUDIO_NO_SOUND;
        }

        return sound_id;
    }

    return AUDIO_NO_SOUND;
}

bool Audio_Stream_Close(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);

    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];

    if (stream->av.codec_ctx) {
        // XXX: potential libav bug - avcodec_close should free this info
        if (stream->av.codec_ctx->extradata != nullptr) {
            av_freep(&stream->av.codec_ctx->extradata);
        }

        avcodec_free_context(&stream->av.codec_ctx);
        stream->av.codec_ctx = nullptr;
    }

    if (stream->av.format_ctx) {
        avformat_close_input(&stream->av.format_ctx);
        stream->av.format_ctx = nullptr;
    }

    if (stream->swr.ctx) {
        swr_free(&stream->swr.ctx);
    }

    if (stream->av.frame) {
        av_frame_free(&stream->av.frame);
        stream->av.frame = nullptr;
    }

    if (stream->av.packet) {
        av_packet_free(&stream->av.packet);
        stream->av.packet = nullptr;
    }

    stream->av.stream = nullptr;
    stream->av.codec = nullptr;

    if (stream->sdl.stream) {
        SDL_FreeAudioStream(stream->sdl.stream);
    }

    void (*finish_callback)(int32_t, void *) = stream->finish_callback;
    void *finish_callback_user_data = stream->finish_callback_user_data;

    M_Clear(stream);

    SDL_UnlockAudioDevice(g_AudioDeviceID);

    if (finish_callback) {
        finish_callback(sound_id, finish_callback_user_data);
    }

    return true;
}

bool Audio_Stream_SetVolume(int32_t sound_id, float volume)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_Streams[sound_id].volume = volume;

    return true;
}

bool Audio_Stream_IsLooped(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    return m_Streams[sound_id].is_looped;
}

bool Audio_Stream_SetIsLooped(int32_t sound_id, bool is_looped)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_Streams[sound_id].is_looped = is_looped;

    return true;
}

bool Audio_Stream_SetFinishCallback(
    int32_t sound_id, void (*callback)(int32_t sound_id, void *user_data),
    void *user_data)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_Streams[sound_id].finish_callback = callback;
    m_Streams[sound_id].finish_callback_user_data = user_data;

    return true;
}

void Audio_Stream_Mix(float *dst_buffer, size_t len)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];
        if (!stream->is_playing) {
            continue;
        }

        while ((SDL_AudioStreamAvailable(stream->sdl.stream) < (int32_t)len)
               && !stream->is_read_done) {
            if (M_DecodeFrame(stream)) {
                M_EnqueueFrame(stream);
            } else {
                stream->is_read_done = true;
            }
        }

        memset(m_MixBuffer, 0, READ_BUFFER_SIZE);
        int32_t bytes_gotten = SDL_AudioStreamGet(
            stream->sdl.stream, m_MixBuffer, READ_BUFFER_SIZE);
        if (bytes_gotten < 0) {
            LOG_ERROR("Error reading from sdl.stream: %s", SDL_GetError());
            stream->is_playing = false;
            stream->is_used = false;
            stream->is_read_done = true;
        } else if (bytes_gotten == 0) {
            // legit end of stream. looping is handled in
            // M_DecodeFrame
            stream->is_playing = false;
            stream->is_used = false;
            stream->is_read_done = true;
        } else {
            int32_t samples_gotten = bytes_gotten
                / (AUDIO_WORKING_CHANNELS * sizeof(AUDIO_WORKING_FORMAT));

            const float *src_ptr = &m_MixBuffer[0];
            float *dst_ptr = dst_buffer;

            const int32_t channels =
                stream->av.codec_ctx->ch_layout.nb_channels;
            if (channels == AUDIO_WORKING_CHANNELS) {
                for (int32_t s = 0; s < samples_gotten; s++) {
                    for (int32_t c = 0; c < AUDIO_WORKING_CHANNELS; c++) {
                        *dst_ptr++ += *src_ptr++ * stream->volume;
                    }
                }
            } else if (channels == 1) {
                for (int32_t s = 0; s < samples_gotten; s++) {
                    for (int32_t c = 0; c < AUDIO_WORKING_CHANNELS; c++) {
                        *dst_ptr++ += *src_ptr * stream->volume;
                    }
                    src_ptr++;
                }
            } else {
                for (int32_t s = 0; s < samples_gotten; s++) {
                    // downmix to mono
                    float src_sample = 0.0f;
                    for (int32_t i = 0; i < channels; i++) {
                        src_sample += *src_ptr++;
                    }
                    src_sample /= (float)channels;
                    for (int32_t c = 0; c < AUDIO_WORKING_CHANNELS; c++) {
                        *dst_ptr++ += src_sample * stream->volume;
                    }
                }
            }
        }

        if (!stream->is_used) {
            Audio_Stream_Close(sound_id);
        }
    }
}

double Audio_Stream_GetTimestamp(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return -1.0;
    }

    double timestamp = -1.0;
    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];

    if (stream->duration > 0.0) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        timestamp = stream->timestamp;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return timestamp;
}

double Audio_Stream_GetDuration(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return -1.0;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];
    double duration = stream->duration;
    SDL_UnlockAudioDevice(g_AudioDeviceID);
    return duration;
}

bool Audio_Stream_SeekTimestamp(int32_t sound_id, double timestamp)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    if (m_Streams[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];
        const double time_base_sec = av_q2d(stream->av.stream->time_base);
        av_seek_frame(
            stream->av.format_ctx, 0, timestamp / time_base_sec,
            AVSEEK_FLAG_ANY);
        avcodec_flush_buffers(stream->av.codec_ctx);
        SDL_UnlockAudioDevice(g_AudioDeviceID);
        return true;
    }

    return false;
}

bool Audio_Stream_SetStartTimestamp(int32_t sound_id, double timestamp)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_Streams[sound_id].start_at = timestamp;
    return true;
}

bool Audio_Stream_SetStopTimestamp(int32_t sound_id, double timestamp)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_Streams[sound_id].stop_at = timestamp;
    return true;
}
