#include <trx/av/audio_internal.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <SDL2/SDL_atomic.h>
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
#include <libavutil/mem.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AVIO_BUFFER_SIZE 4096

// The mixer must never wait for the decoder, and a seek must never discard
// audible audio. 0.37 s of stereo sits comfortably between the two.
#define RING_FLOATS (1 << 15)
#define RING_MASK (RING_FLOATS - 1)
#define RING_REFILL_FLOATS (RING_FLOATS / 2)

typedef enum {
    M_STREAM_SRC_NONE,
    M_STREAM_SRC_MEMORY,
} M_STREAM_SOURCE_TYPE;

typedef struct {
    uint8_t *data;
    size_t size;
    size_t pos;
} M_MEM_SOURCE;

// Written by the worker thread, read by the audio callback, and by nobody
// else. Reset only while the device is locked.
typedef struct {
    float *data;
    SDL_atomic_t read_pos;
    SDL_atomic_t write_pos;
} M_RING;

typedef struct {
    void (*func)(int32_t sound_id, void *user_data);
    void *user_data;
} M_FINISH_NOTIFICATION;

typedef struct {
    bool is_used;
    bool is_playing;
    bool is_read_done;
    bool is_looped;
    float volume;
    double duration;
    double decode_timestamp;
    int64_t played_samples;

    double start_at;
    double stop_at;

    void (*finish_callback)(int32_t sound_id, void *user_data);
    void *finish_callback_user_data;

    M_STREAM_SOURCE_TYPE src_type;
    void *src;
    uint8_t *avio_ctx_buffer;
    AVIOContext *avio_ctx;

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

    M_RING ring;
    SDL_atomic_t is_finished;
    float *convert_buffer;
    size_t convert_capacity;
} AUDIO_STREAM_SOUND;

extern SDL_AudioDeviceID g_AudioDeviceID;

static AUDIO_STREAM_SOUND m_Streams[AUDIO_MAX_ACTIVE_STREAMS] = {};

static uint32_t M_RingUsed(M_RING *const ring)
{
    return (uint32_t)SDL_AtomicGet(&ring->write_pos)
        - (uint32_t)SDL_AtomicGet(&ring->read_pos);
}

static uint32_t M_RingSpace(M_RING *const ring)
{
    return RING_FLOATS - M_RingUsed(ring);
}

static void M_RingWrite(
    M_RING *const ring, const float *const src, uint32_t count)
{
    ASSERT(count <= M_RingSpace(ring));
    count = MIN(count, M_RingSpace(ring));

    const uint32_t write_pos = (uint32_t)SDL_AtomicGet(&ring->write_pos);
    const uint32_t offset = write_pos & RING_MASK;
    const uint32_t head = MIN(count, RING_FLOATS - offset);
    memcpy(ring->data + offset, src, head * sizeof(float));
    memcpy(ring->data, src + head, (count - head) * sizeof(float));

    SDL_MemoryBarrierRelease();
    SDL_AtomicSet(&ring->write_pos, (int32_t)(write_pos + count));
}

static uint32_t M_RingMix(
    M_RING *const ring, float *dst, uint32_t count, const float gain)
{
    count = MIN(count, M_RingUsed(ring));
    SDL_MemoryBarrierAcquire();

    const uint32_t read_pos = (uint32_t)SDL_AtomicGet(&ring->read_pos);
    const uint32_t offset = read_pos & RING_MASK;
    const uint32_t head = MIN(count, RING_FLOATS - offset);

    const float *src = ring->data + offset;
    for (uint32_t i = 0; i < head; i++) {
        *dst++ += *src++ * gain;
    }
    src = ring->data;
    for (uint32_t i = head; i < count; i++) {
        *dst++ += *src++ * gain;
    }

    SDL_AtomicSet(&ring->read_pos, (int32_t)(read_pos + count));
    return count;
}

static void M_RingReset(M_RING *const ring)
{
    SDL_AtomicSet(&ring->read_pos, 0);
    SDL_AtomicSet(&ring->write_pos, 0);
}

static bool M_IsValidID(const int32_t sound_id)
{
    return g_AudioDeviceID != 0 && sound_id >= 0
        && sound_id < AUDIO_MAX_ACTIVE_STREAMS;
}

static int32_t M_MemoryRead(
    void *const opaque, uint8_t *const buf, const int32_t buf_size)
{
    ASSERT(opaque != nullptr);
    ASSERT(buf != nullptr);

    if (buf_size <= 0) {
        return 0;
    }

    M_MEM_SOURCE *const s = opaque;
    if (s->pos >= s->size) {
        return AVERROR_EOF;
    }

    size_t to_copy = s->size - s->pos;
    if (to_copy > (size_t)buf_size) {
        to_copy = (size_t)buf_size;
    }

    memcpy(buf, s->data + s->pos, to_copy);
    s->pos += to_copy;
    return (int32_t)to_copy;
}

static int64_t M_MemorySeek(
    void *const opaque, const int64_t offset, const int32_t whence)
{
    ASSERT(opaque != nullptr);

    M_MEM_SOURCE *const s = opaque;
    if ((whence & AVSEEK_SIZE) != 0) {
        return (int64_t)s->size;
    }

    const int32_t base_whence = whence & ~AVSEEK_FORCE;
    int64_t base;
    if (base_whence == SEEK_SET) {
        base = 0;
    } else if (base_whence == SEEK_CUR) {
        base = (int64_t)s->pos;
    } else if (base_whence == SEEK_END) {
        base = (int64_t)s->size;
    } else {
        return AVERROR(EINVAL);
    }

    int64_t new_pos = base + offset;
    if (new_pos < 0) {
        new_pos = 0;
    }
    if (new_pos > (int64_t)s->size) {
        new_pos = (int64_t)s->size;
    }
    s->pos = (size_t)new_pos;
    return new_pos;
}

static void M_ResetPlaybackState(
    AUDIO_STREAM_SOUND *const stream, const double relative_timestamp)
{
    ASSERT(stream != nullptr);

    const double clamped = MAX(0.0, relative_timestamp);
    Audio_LockDevice();
    stream->played_samples = (int64_t)(clamped * (double)AUDIO_WORKING_RATE);
    Audio_UnlockDevice();
}

static void M_SeekToStart(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    stream->decode_timestamp = stream->start_at;
    M_ResetPlaybackState(stream, 0.0);
    int32_t error_code;
    if (stream->start_at <= 0.0) {
        // reset to start of file
        avio_seek(stream->av.format_ctx->pb, 0, SEEK_SET);
        error_code = avformat_seek_file(
            stream->av.format_ctx, -1, 0, 0, 0, AVSEEK_FLAG_FRAME);
    } else {
        // seek to specific timestamp
        AVFormatContext *const fmt = stream->av.format_ctx;
        if (fmt->pb != nullptr && (fmt->pb->seekable & AVIO_SEEKABLE_NORMAL)) {
            const int64_t ts = (int64_t)(stream->start_at * AV_TIME_BASE);
            error_code = avformat_seek_file(
                fmt, stream->av.stream->index, INT64_MIN, ts, INT64_MAX,
                AVSEEK_FLAG_BACKWARD);
        } else {
            // fallback to stream-based seek
            const double time_base_sec = av_q2d(stream->av.stream->time_base);
            error_code = av_seek_frame(
                fmt, stream->av.stream->index,
                (int64_t)(stream->start_at / time_base_sec), AVSEEK_FLAG_ANY);
        }
    }
    if (error_code < 0) {
        LOG_ERROR(
            "seek failed for timestamp %f: %s", stream->decode_timestamp,
            av_err2str(error_code));
    } else {
        avcodec_flush_buffers(stream->av.codec_ctx);
        stream->is_read_done = false;
    }
}

static bool M_DecodeFrame(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    if (stream->stop_at > 0.0 && stream->decode_timestamp >= stream->stop_at) {
        if (stream->is_looped) {
            M_SeekToStart(stream);
            return M_DecodeFrame(stream);
        } else {
            return false;
        }
    }

    // av_read_frame() overwrites the packet; always unref any previous content.
    av_packet_unref(stream->av.packet);
    int32_t error_code =
        av_read_frame(stream->av.format_ctx, stream->av.packet);

    if (error_code == AVERROR_EOF && stream->is_looped) {
        M_SeekToStart(stream);
        return M_DecodeFrame(stream);
    }

    if (error_code == AVERROR_EOF) {
        return false;
    }

    if (error_code < 0) {
        LOG_ERROR(
            "error while decoding audio stream: %d (%s)", error_code,
            av_err2str(error_code));
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

static bool M_InitialiseResampler(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    stream->swr.src.sample_rate = stream->av.codec_ctx->sample_rate;
    stream->swr.src.ch_layout = stream->av.codec_ctx->ch_layout;
    stream->swr.src.format = stream->av.codec_ctx->sample_fmt;
    stream->swr.dst.sample_rate = AUDIO_WORKING_RATE;
    av_channel_layout_default(
        &stream->swr.dst.ch_layout, AUDIO_WORKING_CHANNELS);
    stream->swr.dst.format = Audio_GetAVAudioFormat(AUDIO_WORKING_FORMAT);

    swr_alloc_set_opts2(
        &stream->swr.ctx, &stream->swr.dst.ch_layout, stream->swr.dst.format,
        stream->swr.dst.sample_rate, &stream->swr.src.ch_layout,
        stream->swr.src.format, stream->swr.src.sample_rate, 0, 0);
    if (stream->swr.ctx == nullptr) {
        LOG_ERROR("Failed to allocate the resampler");
        return false;
    }

    const int32_t error_code = swr_init(stream->swr.ctx);
    if (error_code != 0) {
        LOG_ERROR(
            "Failed to initialise the resampler: %s", av_err2str(error_code));
        swr_free(&stream->swr.ctx);
        return false;
    }

    return true;
}

static float *M_ReserveConvertBuffer(
    AUDIO_STREAM_SOUND *const stream, const int32_t count)
{
    ASSERT(stream != nullptr);

    if ((size_t)count > stream->convert_capacity) {
        stream->convert_capacity = count;
        stream->convert_buffer =
            Memory_Realloc(stream->convert_buffer, count * sizeof(float));
    }
    return stream->convert_buffer;
}

static void M_EnqueueFrame(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    if (stream->swr.ctx == nullptr && !M_InitialiseResampler(stream)) {
        av_packet_unref(stream->av.packet);
        return;
    }

    while (true) {
        AVFrame *const frame = stream->av.frame;
        if (avcodec_receive_frame(stream->av.codec_ctx, frame) < 0) {
            av_frame_unref(frame);
            break;
        }

        const int32_t max_samples =
            swr_get_out_samples(stream->swr.ctx, frame->nb_samples);
        uint8_t *out_buffer = (uint8_t *)M_ReserveConvertBuffer(
            stream, max_samples * AUDIO_WORKING_CHANNELS);
        const int32_t converted = swr_convert(
            stream->swr.ctx, &out_buffer, max_samples,
            (const uint8_t **)frame->data, frame->nb_samples);
        const uint32_t produced = converted * AUDIO_WORKING_CHANNELS;
        if (converted > 0 && produced > M_RingSpace(&stream->ring)) {
            LOG_ERROR("Frame of %d samples does not fit the buffer", converted);
            av_frame_unref(frame);
            break;
        }
        if (converted > 0) {
            M_RingWrite(&stream->ring, stream->convert_buffer, produced);
        }

        const double time_base_sec = av_q2d(stream->av.stream->time_base);
        stream->decode_timestamp = frame->best_effort_timestamp * time_base_sec;
        av_frame_unref(frame);
    }

    av_packet_unref(stream->av.packet);
}

static void M_Refill(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    while (!stream->is_read_done
           && M_RingSpace(&stream->ring) >= RING_REFILL_FLOATS) {
        if (M_DecodeFrame(stream)) {
            M_EnqueueFrame(stream);
        } else {
            stream->is_read_done = true;
        }
    }
}

static M_FINISH_NOTIFICATION M_TakeFinishNotification(
    AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    const M_FINISH_NOTIFICATION notification = {
        .func = stream->finish_callback,
        .user_data = stream->finish_callback_user_data,
    };
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;
    return notification;
}

static void M_Clear(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    stream->is_used = false;
    stream->is_playing = false;
    stream->is_read_done = true;
    stream->is_looped = false;
    stream->volume = 0.0f;
    stream->duration = 0.0;
    stream->decode_timestamp = 0.0;
    stream->played_samples = 0;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;

    stream->src_type = M_STREAM_SRC_NONE;
    stream->src = nullptr;
    stream->avio_ctx_buffer = nullptr;
    stream->avio_ctx = nullptr;

    stream->ring.data = nullptr;
    M_RingReset(&stream->ring);
    SDL_AtomicSet(&stream->is_finished, 0);
    stream->convert_buffer = nullptr;
    stream->convert_capacity = 0;
}

// Tears a stream down. The worker lock must be held; the device lock is taken
// to hide the stream from the mixer before anything it reads is freed.
static void M_Close(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    Audio_LockDevice();
    stream->is_used = false;
    stream->is_playing = false;
    Audio_UnlockDevice();

    if (stream->av.codec_ctx != nullptr) {
        // XXX: potential libav bug - avcodec_close should free this info
        if (stream->av.codec_ctx->extradata != nullptr) {
            av_freep(&stream->av.codec_ctx->extradata);
        }
        avcodec_free_context(&stream->av.codec_ctx);
    }

    if (stream->av.format_ctx != nullptr) {
        avformat_close_input(&stream->av.format_ctx);
    }

    if (stream->avio_ctx != nullptr) {
        av_freep(&stream->avio_ctx->buffer);
        avio_context_free(&stream->avio_ctx);
    } else if (stream->avio_ctx_buffer != nullptr) {
        av_freep(&stream->avio_ctx_buffer);
    }

    if (stream->src_type == M_STREAM_SRC_MEMORY && stream->src != nullptr) {
        M_MEM_SOURCE *const src = stream->src;
        Memory_FreePointer(&src->data);
        Memory_FreePointer(&stream->src);
    }

    if (stream->swr.ctx != nullptr) {
        swr_free(&stream->swr.ctx);
    }

    if (stream->av.frame != nullptr) {
        av_frame_free(&stream->av.frame);
    }

    if (stream->av.packet != nullptr) {
        av_packet_free(&stream->av.packet);
    }

    stream->av.stream = nullptr;
    stream->av.codec = nullptr;

    Memory_FreePointer(&stream->ring.data);
    Memory_FreePointer(&stream->convert_buffer);

    M_Clear(stream);
}

static bool M_InitialiseFromFormatContext(
    const int32_t sound_id, AVFormatContext *const fmt_ctx)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    bool ret = false;
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    int32_t error_code = 0;

    stream->av.format_ctx = fmt_ctx;

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
    if (stream->av.stream == nullptr) {
        error_code = AVERROR_STREAM_NOT_FOUND;
        goto cleanup;
    }

    stream->av.codec =
        avcodec_find_decoder(stream->av.stream->codecpar->codec_id);
    if (stream->av.codec == nullptr) {
        error_code = AVERROR_DEMUXER_NOT_FOUND;
        goto cleanup;
    }

    stream->av.codec_ctx = avcodec_alloc_context3(stream->av.codec);
    if (stream->av.codec_ctx == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    error_code = avcodec_parameters_to_context(
        stream->av.codec_ctx, stream->av.stream->codecpar);
    if (error_code != 0) {
        goto cleanup;
    }

    error_code = avcodec_open2(stream->av.codec_ctx, stream->av.codec, nullptr);
    if (error_code < 0) {
        goto cleanup;
    }

    stream->av.packet = av_packet_alloc();
    if (stream->av.packet == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    stream->av.frame = av_frame_alloc();
    if (stream->av.frame == nullptr) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    stream->ring.data = Memory_Alloc(RING_FLOATS * sizeof(float));
    M_RingReset(&stream->ring);
    SDL_AtomicSet(&stream->is_finished, 0);

    stream->is_read_done = false;
    stream->is_looped = false;
    stream->volume = 1.0f;
    stream->decode_timestamp = 0.0;
    stream->played_samples = 0;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;
    stream->duration =
        (double)stream->av.format_ctx->duration / (double)AV_TIME_BASE;
    stream->start_at = -1.0; // negative value means unset
    stream->stop_at = -1.0; // negative value means unset

    Audio_LockDevice();
    stream->is_used = true;
    stream->is_playing = false;
    Audio_UnlockDevice();

    ret = true;

cleanup:
    if (error_code != 0) {
        LOG_ERROR(
            "Error while opening audio stream: %s", av_err2str(error_code));
    }

    if (!ret) {
        M_Close(stream);
    }

    return ret;
}

static bool M_InitialiseFromPath(
    const int32_t sound_id, const char *const file_path)
{
    ASSERT(file_path != nullptr);

    if (!M_IsValidID(sound_id)) {
        return false;
    }

    AVFormatContext *fmt_ctx = nullptr;
    const int32_t error_code =
        avformat_open_input(&fmt_ctx, file_path, nullptr, nullptr);
    if (error_code != 0) {
        LOG_ERROR(
            "Error while opening audio %s: %s", file_path,
            av_err2str(error_code));
        return false;
    }

    return M_InitialiseFromFormatContext(sound_id, fmt_ctx);
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
    Audio_WorkerLock();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        M_Close(&m_Streams[sound_id]);
    }
    Audio_WorkerUnlock();
}

void Audio_Stream_Pump(void)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        M_FINISH_NOTIFICATION notification = {};

        Audio_WorkerLock();
        AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
        if (stream->is_used) {
            if (SDL_AtomicGet(&stream->is_finished) != 0) {
                notification = M_TakeFinishNotification(stream);
                M_Close(stream);
            } else {
                M_Refill(stream);
            }
        }
        Audio_WorkerUnlock();

        if (notification.func != nullptr) {
            notification.func(sound_id, notification.user_data);
        }
    }
}

bool Audio_Stream_SyncTimestamp(const int32_t sound_id, const double timestamp)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    double drift = Audio_Stream_GetTimestamp(sound_id) - timestamp;
    if (drift < 0) {
        drift = -drift;
    }
    if (drift >= AUDIO_DRIFT_THRESHOLD) {
        LOG_DEBUG("Detected audio drift: %f s", drift);
        Audio_Stream_SeekTimestamp(sound_id, timestamp);
        return true;
    }
    return false;
}

bool Audio_Stream_Pause(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    if (stream->is_playing) {
        Audio_LockDevice();
        stream->is_playing = false;
        Audio_UnlockDevice();
    }

    return true;
}

bool Audio_Stream_Unpause(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    if (!stream->is_playing) {
        Audio_LockDevice();
        stream->is_playing = true;
        Audio_UnlockDevice();
    }

    return true;
}

bool Audio_Stream_SetPaused(const int32_t sound_id, const bool is_paused)
{
    return is_paused ? Audio_Stream_Pause(sound_id)
                     : Audio_Stream_Unpause(sound_id);
}

int32_t Audio_Stream_CreateFromFile(const char *const file_path)
{
    if (g_AudioDeviceID == 0) {
        return AUDIO_NO_SOUND;
    }

    ASSERT(file_path != nullptr);

    int32_t result = AUDIO_NO_SOUND;
    Audio_WorkerLock();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        if (m_Streams[sound_id].is_used) {
            continue;
        }
        if (M_InitialiseFromPath(sound_id, file_path)) {
            result = sound_id;
        }
        break;
    }
    Audio_WorkerUnlock();

    return result;
}

int32_t Audio_Stream_CreateFromMemory(uint8_t *const data, const size_t size)
{
    if (g_AudioDeviceID == 0) {
        return AUDIO_NO_SOUND;
    }

    ASSERT(data != nullptr);
    ASSERT(size != 0);

    int32_t result = AUDIO_NO_SOUND;
    Audio_WorkerLock();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
        if (stream->is_used) {
            continue;
        }

        M_MEM_SOURCE *const src = Memory_Alloc(sizeof(M_MEM_SOURCE));
        *src = (M_MEM_SOURCE) {
            .data = data,
            .size = size,
            .pos = 0,
        };

        stream->src_type = M_STREAM_SRC_MEMORY;
        stream->src = src;

        stream->avio_ctx_buffer = av_malloc(AVIO_BUFFER_SIZE);
        if (stream->avio_ctx_buffer == nullptr) {
            M_Close(stream);
            break;
        }

        stream->avio_ctx = avio_alloc_context(
            stream->avio_ctx_buffer, AVIO_BUFFER_SIZE, 0, src, M_MemoryRead,
            nullptr, M_MemorySeek);
        if (stream->avio_ctx == nullptr) {
            M_Close(stream);
            break;
        }

        stream->av.format_ctx = avformat_alloc_context();
        if (stream->av.format_ctx == nullptr) {
            M_Close(stream);
            break;
        }
        stream->av.format_ctx->pb = stream->avio_ctx;
        stream->av.format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

        const int32_t error_code = avformat_open_input(
            &stream->av.format_ctx, nullptr, nullptr, nullptr);
        if (error_code != 0) {
            LOG_ERROR(
                "Error while opening audio memory stream: %s",
                av_err2str(error_code));
            M_Close(stream);
            break;
        }

        if (M_InitialiseFromFormatContext(sound_id, stream->av.format_ctx)) {
            result = sound_id;
        }
        break;
    }
    Audio_WorkerUnlock();

    return result;
}

bool Audio_Stream_Close(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    Audio_WorkerLock();
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    const M_FINISH_NOTIFICATION notification = M_TakeFinishNotification(stream);
    M_Close(stream);
    Audio_WorkerUnlock();

    if (notification.func != nullptr) {
        notification.func(sound_id, notification.user_data);
    }

    return true;
}

bool Audio_Stream_SetVolume(const int32_t sound_id, const float volume)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    m_Streams[sound_id].volume = volume;

    return true;
}

bool Audio_Stream_IsLooped(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    return m_Streams[sound_id].is_looped;
}

bool Audio_Stream_SetIsLooped(const int32_t sound_id, const bool is_looped)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    m_Streams[sound_id].is_looped = is_looped;

    return true;
}

bool Audio_Stream_SetFinishCallback(
    const int32_t sound_id,
    void (*const callback)(int32_t sound_id, void *user_data),
    void *const user_data)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    Audio_WorkerLock();
    m_Streams[sound_id].finish_callback = callback;
    m_Streams[sound_id].finish_callback_user_data = user_data;
    Audio_WorkerUnlock();

    return true;
}

void Audio_Stream_Mix(float *const dst_buffer, const size_t len)
{
    const uint32_t requested = len / sizeof(float);

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
        if (!stream->is_used || !stream->is_playing) {
            continue;
        }

        const uint32_t mixed =
            M_RingMix(&stream->ring, dst_buffer, requested, stream->volume);
        stream->played_samples += mixed / AUDIO_WORKING_CHANNELS;

        // Looping is handled by the worker, so a dry ring on a stream that has
        // read everything is a legitimate end of playback.
        if (mixed < requested && stream->is_read_done) {
            SDL_AtomicSet(&stream->is_finished, 1);
        }
    }
}

double Audio_Stream_GetTimestamp(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return -1.0;
    }

    double timestamp = -1.0;
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];

    if (stream->duration > 0.0) {
        Audio_LockDevice();
        timestamp = (double)stream->played_samples / (double)AUDIO_WORKING_RATE;
        Audio_UnlockDevice();
    }

    return timestamp;
}

double Audio_Stream_GetDuration(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return -1.0;
    }

    Audio_LockDevice();
    const double duration = m_Streams[sound_id].duration;
    Audio_UnlockDevice();
    return duration;
}

bool Audio_Stream_SeekTimestamp(const int32_t sound_id, const double timestamp)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    Audio_WorkerLock();
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    if (!stream->is_used) {
        Audio_WorkerUnlock();
        return false;
    }

    const double time_base_sec = av_q2d(stream->av.stream->time_base);
    if (time_base_sec <= 0.0) {
        LOG_ERROR(
            "Audio_Stream_SeekTimestamp: invalid time_base %f", time_base_sec);
        Audio_WorkerUnlock();
        return false;
    }

    const int32_t stream_index = stream->av.stream->index;
    const int64_t seek_target =
        (int64_t)((MAX(0.0f, stream->start_at) + timestamp) / time_base_sec);
    const int32_t error_code = av_seek_frame(
        stream->av.format_ctx, stream_index, seek_target, AVSEEK_FLAG_ANY);
    if (error_code < 0) {
        LOG_ERROR(
            "seek failed for timestamp %f: %s", timestamp,
            av_err2str(error_code));
        Audio_WorkerUnlock();
        return false;
    }

    avcodec_flush_buffers(stream->av.codec_ctx);

    Audio_LockDevice();
    M_RingReset(&stream->ring);
    Audio_UnlockDevice();

    stream->decode_timestamp = timestamp + MAX(stream->start_at, 0.0f);
    M_ResetPlaybackState(stream, timestamp);
    stream->is_read_done = false;
    Audio_WorkerUnlock();

    return true;
}

bool Audio_Stream_SetStartTimestamp(
    const int32_t sound_id, const double timestamp)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    m_Streams[sound_id].start_at = timestamp;
    return true;
}

bool Audio_Stream_SetStopTimestamp(
    const int32_t sound_id, const double timestamp)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }

    m_Streams[sound_id].stop_at = timestamp;
    return true;
}
