#include <trx/av/audio_decoder.h>

#include <trx/av/audio_internal.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AVIO_BUFFER_SIZE 4096

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
} M_MEMORY_SOURCE;

struct AUDIO_DECODER {
    int32_t channels;
    double timestamp;
    bool is_drained;

    M_MEMORY_SOURCE memory;
    AVIOContext *avio_ctx;
    AVFormatContext *format_ctx;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    AVPacket *packet;
    AVFrame *frame;
    SwrContext *swr_ctx;

    float *buffer;
    int32_t buffer_capacity;
    int32_t buffer_count;
};

static int32_t M_MemoryRead(
    void *const opaque, uint8_t *const buf, const int32_t buf_size)
{
    ASSERT(opaque != nullptr);
    ASSERT(buf != nullptr);

    if (buf_size <= 0) {
        return 0;
    }

    M_MEMORY_SOURCE *const source = opaque;
    if (source->pos >= source->size) {
        return AVERROR_EOF;
    }

    const size_t to_copy = MIN(source->size - source->pos, (size_t)buf_size);
    memcpy(buf, source->data + source->pos, to_copy);
    source->pos += to_copy;
    return (int32_t)to_copy;
}

static int64_t M_MemorySeek(
    void *const opaque, const int64_t offset, const int32_t whence)
{
    ASSERT(opaque != nullptr);

    M_MEMORY_SOURCE *const source = opaque;
    if ((whence & AVSEEK_SIZE) != 0) {
        return (int64_t)source->size;
    }

    const int32_t base_whence = whence & ~AVSEEK_FORCE;
    int64_t base;
    if (base_whence == SEEK_SET) {
        base = 0;
    } else if (base_whence == SEEK_CUR) {
        base = (int64_t)source->pos;
    } else if (base_whence == SEEK_END) {
        base = (int64_t)source->size;
    } else {
        return AVERROR(EINVAL);
    }

    int64_t new_pos = base + offset;
    CLAMP(new_pos, 0, (int64_t)source->size);
    source->pos = (size_t)new_pos;
    return new_pos;
}

static bool M_OpenCodec(AUDIO_DECODER *const decoder)
{
    int32_t error_code =
        avformat_find_stream_info(decoder->format_ctx, nullptr);
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
        error_code = AVERROR_DECODER_NOT_FOUND;
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
    av_channel_layout_default(&dst_layout, decoder->channels);
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

    return true;

fail:
    LOG_ERROR("Error while opening audio: %s", av_err2str(error_code));
    return false;
}

static void M_Append(
    AUDIO_DECODER *const decoder, const float *const data, const int32_t count)
{
    const int32_t floats = count * decoder->channels;
    if (decoder->buffer_count + floats > decoder->buffer_capacity) {
        decoder->buffer_capacity = decoder->buffer_count + floats;
        decoder->buffer = Memory_Realloc(
            decoder->buffer, decoder->buffer_capacity * sizeof(float));
    }
    memcpy(
        decoder->buffer + decoder->buffer_count, data, floats * sizeof(float));
    decoder->buffer_count += floats;
}

static void M_Convert(AUDIO_DECODER *const decoder, AVFrame *const frame)
{
    const int32_t max_samples =
        swr_get_out_samples(decoder->swr_ctx, frame->nb_samples);
    if (max_samples <= 0) {
        return;
    }

    const int32_t floats = max_samples * decoder->channels;
    float *const scratch = Memory_Alloc(floats * sizeof(float));
    uint8_t *out_buffer = (uint8_t *)scratch;
    const int32_t converted = swr_convert(
        decoder->swr_ctx, &out_buffer, max_samples,
        (const uint8_t **)frame->data, frame->nb_samples);
    if (converted > 0) {
        M_Append(decoder, scratch, converted);
    }
    Memory_Free(scratch);
}

static void M_Drain(AUDIO_DECODER *const decoder)
{
    while (avcodec_receive_frame(decoder->codec_ctx, decoder->frame) >= 0) {
        M_Convert(decoder, decoder->frame);
        decoder->timestamp = decoder->frame->best_effort_timestamp
            * av_q2d(decoder->stream->time_base);
        av_frame_unref(decoder->frame);
    }
    av_frame_unref(decoder->frame);
}

static AUDIO_DECODER *M_Create(const int32_t channels)
{
    AUDIO_DECODER *const decoder = Memory_Alloc(sizeof(AUDIO_DECODER));
    decoder->channels = channels;
    return decoder;
}

AUDIO_DECODER *AudioDecoder_CreateFromPath(
    const char *const path, const int32_t channels)
{
    ASSERT(path != nullptr);

    AUDIO_DECODER *decoder = M_Create(channels);
    const int32_t error_code =
        avformat_open_input(&decoder->format_ctx, path, nullptr, nullptr);
    if (error_code != 0) {
        LOG_ERROR(
            "Error while opening audio %s: %s", path, av_err2str(error_code));
        AudioDecoder_Free(&decoder);
        return nullptr;
    }

    if (!M_OpenCodec(decoder)) {
        AudioDecoder_Free(&decoder);
        return nullptr;
    }
    return decoder;
}

AUDIO_DECODER *AudioDecoder_CreateFromMemory(
    const uint8_t *const data, const size_t size, const int32_t channels)
{
    ASSERT(data != nullptr);
    ASSERT(size != 0);

    AUDIO_DECODER *decoder = M_Create(channels);
    decoder->memory = (M_MEMORY_SOURCE) {
        .data = data,
        .size = size,
        .pos = 0,
    };

    decoder->avio_ctx = avio_alloc_context(
        av_malloc(AVIO_BUFFER_SIZE), AVIO_BUFFER_SIZE, 0, &decoder->memory,
        M_MemoryRead, nullptr, M_MemorySeek);
    if (decoder->avio_ctx == nullptr) {
        AudioDecoder_Free(&decoder);
        return nullptr;
    }

    decoder->format_ctx = avformat_alloc_context();
    if (decoder->format_ctx == nullptr) {
        AudioDecoder_Free(&decoder);
        return nullptr;
    }
    decoder->format_ctx->pb = decoder->avio_ctx;
    decoder->format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    const int32_t error_code =
        avformat_open_input(&decoder->format_ctx, nullptr, nullptr, nullptr);
    if (error_code != 0) {
        LOG_ERROR(
            "Error while opening audio memory stream: %s",
            av_err2str(error_code));
        AudioDecoder_Free(&decoder);
        return nullptr;
    }

    if (!M_OpenCodec(decoder)) {
        AudioDecoder_Free(&decoder);
        return nullptr;
    }
    return decoder;
}

void AudioDecoder_Free(AUDIO_DECODER **const decoder_ref)
{
    AUDIO_DECODER *const decoder = *decoder_ref;
    if (decoder == nullptr) {
        return;
    }

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
        // XXX: potential libav bug - avcodec_close should free this info
        if (decoder->codec_ctx->extradata != nullptr) {
            av_freep(&decoder->codec_ctx->extradata);
        }
        avcodec_free_context(&decoder->codec_ctx);
    }
    if (decoder->format_ctx != nullptr) {
        avformat_close_input(&decoder->format_ctx);
    }
    if (decoder->avio_ctx != nullptr) {
        av_freep(&decoder->avio_ctx->buffer);
        avio_context_free(&decoder->avio_ctx);
    }
    Memory_FreePointer(&decoder->buffer);
    Memory_FreePointer(decoder_ref);
}

double AudioDecoder_GetDuration(const AUDIO_DECODER *const decoder)
{
    ASSERT(decoder != nullptr);
    if (decoder->format_ctx->duration <= 0) {
        return -1.0;
    }
    return (double)decoder->format_ctx->duration / (double)AV_TIME_BASE;
}

double AudioDecoder_GetTimestamp(const AUDIO_DECODER *const decoder)
{
    ASSERT(decoder != nullptr);
    return decoder->timestamp;
}

bool AudioDecoder_Seek(AUDIO_DECODER *const decoder, const double timestamp)
{
    ASSERT(decoder != nullptr);

    const double time_base_sec = av_q2d(decoder->stream->time_base);
    if (time_base_sec <= 0.0) {
        LOG_ERROR("Invalid time base %f", time_base_sec);
        return false;
    }

    const int32_t error_code = av_seek_frame(
        decoder->format_ctx, decoder->stream->index,
        (int64_t)(timestamp / time_base_sec), AVSEEK_FLAG_ANY);
    if (error_code < 0) {
        LOG_ERROR(
            "seek failed for timestamp %f: %s", timestamp,
            av_err2str(error_code));
        return false;
    }

    avcodec_flush_buffers(decoder->codec_ctx);
    decoder->timestamp = timestamp;
    decoder->is_drained = false;
    return true;
}

bool AudioDecoder_Rewind(AUDIO_DECODER *const decoder, const double start_at)
{
    ASSERT(decoder != nullptr);

    int32_t error_code;
    if (start_at <= 0.0) {
        avio_seek(decoder->format_ctx->pb, 0, SEEK_SET);
        error_code = avformat_seek_file(
            decoder->format_ctx, -1, 0, 0, 0, AVSEEK_FLAG_FRAME);
    } else if (
        decoder->format_ctx->pb != nullptr
        && (decoder->format_ctx->pb->seekable & AVIO_SEEKABLE_NORMAL) != 0) {
        const int64_t ts = (int64_t)(start_at * AV_TIME_BASE);
        error_code = avformat_seek_file(
            decoder->format_ctx, decoder->stream->index, INT64_MIN, ts,
            INT64_MAX, AVSEEK_FLAG_BACKWARD);
    } else {
        return AudioDecoder_Seek(decoder, start_at);
    }

    if (error_code < 0) {
        LOG_ERROR(
            "seek failed for timestamp %f: %s", start_at,
            av_err2str(error_code));
        return false;
    }

    avcodec_flush_buffers(decoder->codec_ctx);
    decoder->timestamp = start_at;
    decoder->is_drained = false;
    return true;
}

int32_t AudioDecoder_Read(AUDIO_DECODER *const decoder, const float **const out)
{
    ASSERT(decoder != nullptr);
    ASSERT(out != nullptr);

    decoder->buffer_count = 0;
    *out = decoder->buffer;

    if (decoder->is_drained) {
        return -1;
    }

    av_packet_unref(decoder->packet);
    const int32_t error_code =
        av_read_frame(decoder->format_ctx, decoder->packet);
    if (error_code < 0) {
        if (error_code != AVERROR_EOF) {
            LOG_ERROR("Error while reading audio: %s", av_err2str(error_code));
        }
        // let the codec hand back whatever it was still holding
        avcodec_send_packet(decoder->codec_ctx, nullptr);
        M_Drain(decoder);
        decoder->is_drained = true;
        *out = decoder->buffer;
        return decoder->buffer_count > 0
            ? decoder->buffer_count / decoder->channels
            : -1;
    }

    if (decoder->packet->stream_index == decoder->stream->index
        && avcodec_send_packet(decoder->codec_ctx, decoder->packet) >= 0) {
        M_Drain(decoder);
    }
    av_packet_unref(decoder->packet);

    *out = decoder->buffer;
    return decoder->buffer_count / decoder->channels;
}
