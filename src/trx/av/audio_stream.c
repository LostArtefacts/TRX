#include <trx/av/audio_internal.h>

#include <trx/av/audio_decoder.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <SDL2/SDL_atomic.h>
#include <stdint.h>
#include <string.h>

// The mixer must never wait for the decoder, and a seek must never discard
// audible audio. 0.37 s of stereo sits comfortably between the two.
#define RING_FLOATS (1 << 15)
#define RING_MASK (RING_FLOATS - 1)
#define RING_REFILL_FLOATS (RING_FLOATS / 2)

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
    double speed;
    double duration;
    double decode_timestamp;
    int64_t played_samples;

    double start_at;
    double stop_at;

    void (*finish_callback)(int32_t sound_id, void *user_data);
    void *finish_callback_user_data;

    AUDIO_DECODER *decoder;
    // what the last read produced but the ring had no room for; it stays
    // valid until the decoder is read again
    const float *pending;
    uint32_t pending_count;
    // the memory a stream was created over, which it then owns
    uint8_t *memory;

    M_RING ring;
    SDL_atomic_t is_finished;
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

// Writes what there is room for and reports it, so that the caller decides
// what to do with a block the ring cannot take in one go.
static uint32_t M_RingWrite(
    M_RING *const ring, const float *const src, uint32_t count)
{
    count = MIN(count, M_RingSpace(ring));

    const uint32_t write_pos = (uint32_t)SDL_AtomicGet(&ring->write_pos);
    const uint32_t offset = write_pos & RING_MASK;
    const uint32_t head = MIN(count, RING_FLOATS - offset);
    memcpy(ring->data + offset, src, head * sizeof(float));
    memcpy(ring->data, src + head, (count - head) * sizeof(float));

    SDL_MemoryBarrierRelease();
    SDL_AtomicSet(&ring->write_pos, (int32_t)(write_pos + count));
    return count;
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

static RESULT M_CheckID(const int32_t sound_id)
{
    MUST(Audio_CheckDevice());
    FAIL_IF(
        sound_id < 0 || sound_id >= AUDIO_MAX_ACTIVE_STREAMS,
        "stream %d is not playing", sound_id);
    return OK;
}

static void M_ResetPlaybackState(
    AUDIO_STREAM_SOUND *const stream, const double relative_timestamp)
{
    ASSERT(stream != nullptr);
    const double clamped = MAX(0.0, relative_timestamp);
    Audio_LockDevice();
    stream->played_samples =
        (int64_t)(clamped * (double)AUDIO_WORKING_RATE / stream->speed);
    Audio_UnlockDevice();
}

static bool M_Rewind(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);
    if (!stream->is_looped
        || !SHOULD(AudioDecoder_Rewind(stream->decoder, stream->start_at))) {
        return false;
    }
    stream->decode_timestamp = MAX(stream->start_at, 0.0);
    M_ResetPlaybackState(stream, 0.0);
    return true;
}

static void M_Refill(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    bool has_rewound = false;
    while (!stream->is_read_done) {
        if (stream->pending_count > 0) {
            const uint32_t written = M_RingWrite(
                &stream->ring, stream->pending, stream->pending_count);
            stream->pending += written;
            stream->pending_count -= written;
            if (stream->pending_count > 0) {
                // the rest waits for the mixer to make room
                break;
            }
            continue;
        }

        if (M_RingSpace(&stream->ring) < RING_REFILL_FLOATS) {
            break;
        }

        const bool is_past_stop = stream->stop_at > 0.0
            && stream->decode_timestamp >= stream->stop_at;

        const float *samples = nullptr;
        const int32_t count =
            is_past_stop ? -1 : AudioDecoder_Read(stream->decoder, &samples);

        if (count < 0) {
            // a loop that hands back nothing twice over has nothing to play
            if (has_rewound || !M_Rewind(stream)) {
                stream->is_read_done = true;
                break;
            }
            has_rewound = true;
            continue;
        }

        if (count > 0) {
            stream->pending = samples;
            stream->pending_count = count * AUDIO_WORKING_CHANNELS;
            has_rewound = false;
        }
        stream->decode_timestamp = AudioDecoder_GetTimestamp(stream->decoder);
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
    stream->speed = 1.0;
    stream->duration = 0.0;
    stream->decode_timestamp = 0.0;
    stream->played_samples = 0;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;

    stream->decoder = nullptr;
    stream->memory = nullptr;
    stream->pending = nullptr;
    stream->pending_count = 0;
    stream->ring.data = nullptr;
    M_RingReset(&stream->ring);
    SDL_AtomicSet(&stream->is_finished, 0);
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

    AudioDecoder_Free(&stream->decoder);
    Memory_FreePointer(&stream->memory);
    Memory_FreePointer(&stream->ring.data);

    M_Clear(stream);
}

// Takes over the decoder, and the memory it reads, either way it turns out.
static bool M_Initialise(
    const int32_t sound_id, AUDIO_DECODER *const decoder, uint8_t *const memory)
{
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    stream->decoder = decoder;
    stream->memory = memory;

    if (decoder == nullptr) {
        M_Close(stream);
        return false;
    }

    stream->ring.data = Memory_Alloc(RING_FLOATS * sizeof(float));
    M_RingReset(&stream->ring);
    SDL_AtomicSet(&stream->is_finished, 0);

    stream->is_read_done = false;
    stream->is_looped = false;
    stream->volume = 1.0f;
    stream->speed = 1.0;
    stream->decode_timestamp = 0.0;
    stream->played_samples = 0;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;
    stream->duration = AudioDecoder_GetDuration(decoder);
    stream->start_at = -1.0; // negative value means unset
    stream->stop_at = -1.0; // negative value means unset

    Audio_LockDevice();
    stream->is_used = true;
    stream->is_playing = false;
    Audio_UnlockDevice();

    return true;
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

RESULT Audio_Stream_SyncTimestamp(
    const int32_t sound_id, const double timestamp)
{
    MUST(M_CheckID(sound_id));
    double drift = Audio_Stream_GetTimestamp(sound_id) - timestamp;
    if (drift < 0) {
        drift = -drift;
    }
    if (drift >= AUDIO_DRIFT_THRESHOLD) {
        LOG_DEBUG("Detected audio drift: %f s", drift);
        MUST(Audio_Stream_SeekTimestamp(sound_id, timestamp));
    }
    return OK;
}

RESULT Audio_Stream_Pause(const int32_t sound_id)
{
    MUST(M_CheckID(sound_id));
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    if (stream->is_playing) {
        Audio_LockDevice();
        stream->is_playing = false;
        Audio_UnlockDevice();
    }
    return OK;
}

RESULT Audio_Stream_Unpause(const int32_t sound_id)
{
    MUST(M_CheckID(sound_id));
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    if (!stream->is_playing) {
        Audio_LockDevice();
        stream->is_playing = true;
        Audio_UnlockDevice();
    }
    return OK;
}

RESULT Audio_Stream_SetPaused(const int32_t sound_id, const bool is_paused)
{
    return is_paused ? Audio_Stream_Pause(sound_id)
                     : Audio_Stream_Unpause(sound_id);
}

RESULT Audio_Stream_CreateFromFile(
    const char *const file_path, int32_t *const out_sound_id)
{
    *out_sound_id = AUDIO_NO_SOUND;
    FAIL_IF(g_AudioDeviceID == 0, "the audio device is not open");

    ASSERT(file_path != nullptr);

    RESULT result = OK;
    bool have_slot = false;
    Audio_WorkerLock();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        if (m_Streams[sound_id].is_used) {
            continue;
        }
        have_slot = true;
        AUDIO_DECODER *decoder = nullptr;
        result = AudioDecoder_CreateFromPath(
            file_path, AUDIO_WORKING_CHANNELS, &decoder);
        if (IS_OK(result) && M_Initialise(sound_id, decoder, nullptr)) {
            *out_sound_id = sound_id;
        } else if (IS_OK(result)) {
            result = FAIL("%s: the stream could not be set up", file_path);
        }
        break;
    }
    Audio_WorkerUnlock();

    if (!have_slot) {
        result = FAIL(
            "%s: all %d streams are in use", file_path,
            AUDIO_MAX_ACTIVE_STREAMS);
    }
    return result;
}

RESULT Audio_Stream_CreateFromMemory(
    uint8_t *const data, const size_t size, int32_t *const out_sound_id)
{
    *out_sound_id = AUDIO_NO_SOUND;
    FAIL_IF(g_AudioDeviceID == 0, "the audio device is not open");

    ASSERT(data != nullptr);
    ASSERT(size != 0);

    RESULT result = OK;
    bool have_slot = false;
    Audio_WorkerLock();
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        if (m_Streams[sound_id].is_used) {
            continue;
        }
        have_slot = true;
        AUDIO_DECODER *decoder = nullptr;
        result = AudioDecoder_CreateFromMemory(
            data, size, AUDIO_WORKING_CHANNELS, &decoder);
        if (IS_OK(result) && M_Initialise(sound_id, decoder, data)) {
            *out_sound_id = sound_id;
        } else if (IS_OK(result)) {
            result = FAIL("the stream could not be set up");
        }
        break;
    }
    Audio_WorkerUnlock();

    if (!have_slot) {
        result = FAIL("all %d streams are in use", AUDIO_MAX_ACTIVE_STREAMS);
    }
    return result;
}

RESULT Audio_Stream_Close(const int32_t sound_id)
{
    MUST(M_CheckID(sound_id));

    Audio_WorkerLock();
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    const M_FINISH_NOTIFICATION notification = M_TakeFinishNotification(stream);
    M_Close(stream);
    Audio_WorkerUnlock();

    if (notification.func != nullptr) {
        notification.func(sound_id, notification.user_data);
    }

    return OK;
}

RESULT Audio_Stream_SetVolume(const int32_t sound_id, const float volume)
{
    MUST(M_CheckID(sound_id));
    m_Streams[sound_id].volume = volume;
    return OK;
}

RESULT Audio_Stream_SetSpeed(const int32_t sound_id, const double speed)
{
    MUST(M_CheckID(sound_id));
    FAIL_IF(speed <= 0.0, "a stream cannot play at %f times its rate", speed);

    Audio_WorkerLock();
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    RESULT result = OK;
    if (stream->is_used) {
        result = AudioDecoder_SetSpeed(stream->decoder, speed);
        // a rate the decoder cannot reach leaves it playing at its own
        stream->speed = IS_OK(result) ? speed : 1.0;
    }
    Audio_WorkerUnlock();

    MUST(result, "stream %d", sound_id);
    return OK;
}

bool Audio_Stream_IsLooped(const int32_t sound_id)
{
    if (!M_IsValidID(sound_id)) {
        return false;
    }
    return m_Streams[sound_id].is_looped;
}

RESULT Audio_Stream_SetIsLooped(const int32_t sound_id, const bool is_looped)
{
    MUST(M_CheckID(sound_id));
    m_Streams[sound_id].is_looped = is_looped;
    return OK;
}

RESULT Audio_Stream_SetFinishCallback(
    const int32_t sound_id,
    void (*const callback)(int32_t sound_id, void *user_data),
    void *const user_data)
{
    MUST(M_CheckID(sound_id));
    Audio_WorkerLock();
    m_Streams[sound_id].finish_callback = callback;
    m_Streams[sound_id].finish_callback_user_data = user_data;
    Audio_WorkerUnlock();
    return OK;
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
        timestamp = (double)stream->played_samples * stream->speed
            / (double)AUDIO_WORKING_RATE;
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

RESULT Audio_Stream_SeekTimestamp(
    const int32_t sound_id, const double timestamp)
{
    MUST(M_CheckID(sound_id));

    Audio_WorkerLock();
    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    RESULT result = FAIL("stream %d holds no audio", sound_id);
    if (stream->is_used) {
        const double target = MAX(stream->start_at, 0.0) + timestamp;
        result = AudioDecoder_Seek(stream->decoder, target);
    }

    if (IS_OK(result)) {
        Audio_LockDevice();
        M_RingReset(&stream->ring);
        Audio_UnlockDevice();

        stream->pending_count = 0;
        stream->decode_timestamp = timestamp + MAX(stream->start_at, 0.0);
        M_ResetPlaybackState(stream, timestamp);
        stream->is_read_done = false;
    }
    Audio_WorkerUnlock();

    MUST(result, "stream %d", sound_id);
    return OK;
}

RESULT Audio_Stream_SetStartTimestamp(
    const int32_t sound_id, const double timestamp)
{
    MUST(M_CheckID(sound_id));
    m_Streams[sound_id].start_at = timestamp;
    return OK;
}

RESULT Audio_Stream_SetStopTimestamp(
    const int32_t sound_id, const double timestamp)
{
    MUST(M_CheckID(sound_id));
    m_Streams[sound_id].stop_at = timestamp;
    return OK;
}
