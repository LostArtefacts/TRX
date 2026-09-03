#include <trx/av/audio_decoder.h>

#include <trx/av/audio_internal.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <errno.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
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

    double speed;
    AVFilterGraph *filter_graph;
    AVFilterContext *filter_src;
    AVFilterContext *filter_sink;
    AVFrame *filter_in;
    AVFrame *filter_out;

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

static RESULT M_OpenCodec(AUDIO_DECODER *const decoder)
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

    return OK;

fail:
    return FAIL("the audio could not be opened: %s", av_err2str(error_code));
}

static void M_FreeFilterGraph(AUDIO_DECODER *const decoder)
{
    if (decoder->filter_graph != nullptr) {
        avfilter_graph_free(&decoder->filter_graph);
    }
    if (decoder->filter_in != nullptr) {
        av_frame_free(&decoder->filter_in);
    }
    if (decoder->filter_out != nullptr) {
        av_frame_free(&decoder->filter_out);
    }
    decoder->filter_src = nullptr;
    decoder->filter_sink = nullptr;
}

static RESULT M_BuildFilterGraph(AUDIO_DECODER *const decoder)
{
    decoder->filter_graph = avfilter_graph_alloc();
    decoder->filter_in = av_frame_alloc();
    decoder->filter_out = av_frame_alloc();
    FAIL_IF(
        decoder->filter_graph == nullptr || decoder->filter_in == nullptr
            || decoder->filter_out == nullptr,
        "the filter graph could not be set up");

    AVChannelLayout layout;
    av_channel_layout_default(&layout, decoder->channels);
    char layout_name[64];
    av_channel_layout_describe(&layout, layout_name, sizeof(layout_name));

    char *const args = String_Format(
        "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
        AUDIO_WORKING_RATE, AUDIO_WORKING_RATE,
        av_get_sample_fmt_name(AV_SAMPLE_FMT_FLT), layout_name);
    const int32_t error_code = avfilter_graph_create_filter(
        &decoder->filter_src, avfilter_get_by_name("abuffer"), "in", args,
        nullptr, decoder->filter_graph);
    Memory_Free(args);
    FAIL_IF(
        error_code < 0, "the filter source could not be created: %s",
        av_err2str(error_code));

    decoder->filter_sink = avfilter_graph_alloc_filter(
        decoder->filter_graph, avfilter_get_by_name("abuffersink"), "out");
    FAIL_IF(
        decoder->filter_sink == nullptr,
        "the filter sink could not be created");

    // the sink only takes its format before it is initialised
    static const enum AVSampleFormat sample_fmts[] = {
        AV_SAMPLE_FMT_FLT,
        AV_SAMPLE_FMT_NONE,
    };
    if (av_opt_set_bin(
            decoder->filter_sink, "sample_fmts", (const uint8_t *)sample_fmts,
            sizeof(sample_fmts), AV_OPT_SEARCH_CHILDREN)
            < 0
        || avfilter_init_dict(decoder->filter_sink, nullptr) < 0) {
        return FAIL("the filter output format would not hold");
    }

    char *const tempo_args = String_Format("tempo=%f", decoder->speed);
    AVFilterContext *stretch = nullptr;
    const int32_t stretch_result = avfilter_graph_create_filter(
        &stretch, avfilter_get_by_name("rubberband"), "stretch", tempo_args,
        nullptr, decoder->filter_graph);
    Memory_Free(tempo_args);
    if (stretch_result < 0
        || avfilter_link(decoder->filter_src, 0, stretch, 0) < 0) {
        return FAIL("the time stretch filter could not be created");
    }

    if (avfilter_link(stretch, 0, decoder->filter_sink, 0) < 0
        || avfilter_graph_config(decoder->filter_graph, nullptr) < 0) {
        return FAIL("the filter graph could not be configured");
    }

    return OK;
}

// Drops the samples the stretcher holds, which a seek makes stale.
static void M_RebuildFilterGraph(AUDIO_DECODER *const decoder)
{
    if (decoder->filter_graph == nullptr) {
        return;
    }
    M_FreeFilterGraph(decoder);
    if (!SHOULD(M_BuildFilterGraph(decoder))) {
        M_FreeFilterGraph(decoder);
        decoder->speed = 1.0;
    }
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

static void M_Filter(
    AUDIO_DECODER *const decoder, const float *const data, const int32_t count)
{
    AVFrame *const in = decoder->filter_in;
    av_frame_unref(in);
    in->format = AV_SAMPLE_FMT_FLT;
    in->sample_rate = AUDIO_WORKING_RATE;
    in->nb_samples = count;
    av_channel_layout_default(&in->ch_layout, decoder->channels);
    if (av_frame_get_buffer(in, 0) < 0) {
        return;
    }
    memcpy(in->data[0], data, count * decoder->channels * sizeof(float));
    if (av_buffersrc_add_frame(decoder->filter_src, in) < 0) {
        return;
    }

    AVFrame *const out = decoder->filter_out;
    while (av_buffersink_get_frame(decoder->filter_sink, out) >= 0) {
        M_Append(decoder, (const float *)out->data[0], out->nb_samples);
        av_frame_unref(out);
    }
}

// Pushes the samples the resampler still holds through to the sink.
static void M_FlushResampler(AUDIO_DECODER *const decoder)
{
    if (decoder->swr_ctx == nullptr) {
        return;
    }

    while (true) {
        const int32_t max_samples = swr_get_out_samples(decoder->swr_ctx, 0);
        if (max_samples <= 0) {
            break;
        }

        const int32_t floats = max_samples * decoder->channels;
        float *const scratch = Memory_Alloc(floats * sizeof(float));
        uint8_t *out_buffer = (uint8_t *)scratch;
        const int32_t converted =
            swr_convert(decoder->swr_ctx, &out_buffer, max_samples, nullptr, 0);
        if (converted > 0) {
            if (decoder->filter_graph != nullptr) {
                M_Filter(decoder, scratch, converted);
            } else {
                M_Append(decoder, scratch, converted);
            }
        }
        Memory_Free(scratch);

        if (converted <= 0) {
            break;
        }
    }
}

// Pushes the samples the stretcher still holds through to the sink.
static void M_FlushFilter(AUDIO_DECODER *const decoder)
{
    if (decoder->filter_graph == nullptr) {
        return;
    }

    if (av_buffersrc_add_frame(decoder->filter_src, nullptr) < 0) {
        LOG_WARNING("Failed to flush the audio filter");
    }
    AVFrame *const out = decoder->filter_out;
    while (av_buffersink_get_frame(decoder->filter_sink, out) >= 0) {
        M_Append(decoder, (const float *)out->data[0], out->nb_samples);
        av_frame_unref(out);
    }
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
        if (decoder->filter_graph != nullptr) {
            M_Filter(decoder, scratch, converted);
        } else {
            M_Append(decoder, scratch, converted);
        }
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
    decoder->speed = 1.0;
    return decoder;
}

RESULT AudioDecoder_CreateFromPath(
    const char *const path, const int32_t channels,
    AUDIO_DECODER **const out_decoder)
{
    ASSERT(path != nullptr);
    *out_decoder = nullptr;

    AUDIO_DECODER *decoder = M_Create(channels);
    const int32_t error_code =
        avformat_open_input(&decoder->format_ctx, path, nullptr, nullptr);
    if (error_code != 0) {
        AudioDecoder_Free(&decoder);
        return FAIL("%s: %s", path, av_err2str(error_code));
    }

    const RESULT result = Result_Prefix(M_OpenCodec(decoder), "%s", path);
    if (!IS_OK(result)) {
        AudioDecoder_Free(&decoder);
        return result;
    }
    *out_decoder = decoder;
    return OK;
}

RESULT AudioDecoder_CreateFromMemory(
    const uint8_t *const data, const size_t size, const int32_t channels,
    AUDIO_DECODER **const out_decoder)
{
    ASSERT(data != nullptr);
    ASSERT(size != 0);
    *out_decoder = nullptr;

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
        return FAIL("the audio reader could not be set up");
    }

    decoder->format_ctx = avformat_alloc_context();
    if (decoder->format_ctx == nullptr) {
        AudioDecoder_Free(&decoder);
        return FAIL("the audio format context could not be set up");
    }
    decoder->format_ctx->pb = decoder->avio_ctx;
    decoder->format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    const int32_t error_code =
        avformat_open_input(&decoder->format_ctx, nullptr, nullptr, nullptr);
    if (error_code != 0) {
        AudioDecoder_Free(&decoder);
        return FAIL(
            "the audio in memory could not be opened: %s",
            av_err2str(error_code));
    }

    const RESULT result = M_OpenCodec(decoder);
    if (!IS_OK(result)) {
        AudioDecoder_Free(&decoder);
        return result;
    }
    *out_decoder = decoder;
    return OK;
}

void AudioDecoder_Free(AUDIO_DECODER **const decoder_ref)
{
    AUDIO_DECODER *const decoder = *decoder_ref;
    if (decoder == nullptr) {
        return;
    }

    M_FreeFilterGraph(decoder);
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

RESULT AudioDecoder_SetSpeed(AUDIO_DECODER *const decoder, const double speed)
{
    ASSERT(decoder != nullptr);

    FAIL_IF(speed <= 0.0, "audio cannot decode at %f times its rate", speed);
    if (speed == decoder->speed) {
        return OK;
    }

    M_FreeFilterGraph(decoder);
    decoder->speed = speed;
    if (speed == 1.0) {
        return OK;
    }

    const RESULT result = M_BuildFilterGraph(decoder);
    if (!IS_OK(result)) {
        M_FreeFilterGraph(decoder);
        decoder->speed = 1.0;
    }
    return result;
}

RESULT AudioDecoder_Seek(AUDIO_DECODER *const decoder, const double timestamp)
{
    ASSERT(decoder != nullptr);

    const double time_base_sec = av_q2d(decoder->stream->time_base);
    FAIL_IF(
        time_base_sec <= 0.0, "the audio names a time base of %f",
        time_base_sec);

    const int32_t error_code = av_seek_frame(
        decoder->format_ctx, decoder->stream->index,
        (int64_t)(timestamp / time_base_sec), AVSEEK_FLAG_ANY);
    FAIL_IF(
        error_code < 0, "the audio could not seek to %f s: %s", timestamp,
        av_err2str(error_code));

    avcodec_flush_buffers(decoder->codec_ctx);
    M_RebuildFilterGraph(decoder);
    decoder->timestamp = timestamp;
    decoder->is_drained = false;
    return OK;
}

RESULT AudioDecoder_Rewind(AUDIO_DECODER *const decoder, const double start_at)
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

    FAIL_IF(
        error_code < 0, "the audio could not seek to %f s: %s", start_at,
        av_err2str(error_code));

    avcodec_flush_buffers(decoder->codec_ctx);
    M_RebuildFilterGraph(decoder);
    decoder->timestamp = start_at;
    decoder->is_drained = false;
    return OK;
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
        // let the codec hand back the frames it was still holding
        avcodec_send_packet(decoder->codec_ctx, nullptr);
        M_Drain(decoder);
        M_FlushResampler(decoder);
        M_FlushFilter(decoder);
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
