#ifdef EMSCRIPTEN_BUILD

    #define DR_FLAC_IMPLEMENTATION
    #define DR_FLAC_NO_STDIO
    #include "vendor/dr_flac.h"

    #define DR_MP3_IMPLEMENTATION
    #define DR_MP3_NO_STDIO
    #include "vendor/dr_mp3.h"

    // stb_vorbis needs its implementation compiled in; we use the pushdata API
    // with STB_VORBIS_NO_STDIO to avoid file I/O deps.
    #define STB_VORBIS_NO_STDIO
    #include "vendor/stb_vorbis.c"

    // dr_wav is already implemented in audio_sample_emscripten.c, just need
    // header.
    #define DR_WAV_NO_STDIO
    #include "vendor/dr_wav.h"

    #include <trx/av/audio_internal.h>

    #include <trx/debug.h>
    #include <trx/core/filesystem.h>
    #include <trx/core/log.h>
    #include <trx/core/memory.h>
    #include <trx/core/utils.h>

    #include <SDL2/SDL_audio.h>
    #include <SDL2/SDL_error.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <string.h>

    #define READ_BUFFER_SIZE                                                   \
        (AUDIO_SAMPLES * AUDIO_WORKING_CHANNELS * sizeof(AUDIO_WORKING_FORMAT))

    // Decode buffer: how many float frames to decode per Mix iteration.
    #define DECODE_FRAMES 2048

typedef enum {
    M_FMT_UNKNOWN,
    M_FMT_WAV,
    M_FMT_FLAC,
    M_FMT_MP3,
    M_FMT_OGG,
} M_FORMAT;

typedef struct {
    M_FORMAT format;
    int32_t sample_rate;
    int32_t channels;
    double duration;

    union {
        drwav wav;
        drflac *flac;
        drmp3 mp3;
        stb_vorbis *ogg;
    } ctx;

    // Memory-backed source data (kept alive for the decoder's lifetime).
    uint8_t *mem_data;
    size_t mem_size;
} M_DECODER;

typedef struct {
    bool is_used;
    bool is_playing;
    bool is_read_done;
    bool is_looped;
    float volume;
    double duration;
    int64_t played_samples;

    double start_at;
    double stop_at;

    void (*finish_callback)(int32_t sound_id, void *user_data);
    void *finish_callback_user_data;

    M_DECODER decoder;
    SDL_AudioStream *sdl_stream;
} AUDIO_STREAM_SOUND;

static AUDIO_STREAM_SOUND m_Streams[AUDIO_MAX_ACTIVE_STREAMS] = {};
static float m_MixBuffer[AUDIO_SAMPLES * AUDIO_WORKING_CHANNELS] = {};
static float m_DecodeBuffer[DECODE_FRAMES * 2] = {};

// ---------------------------------------------------------------------------
// Format detection
// ---------------------------------------------------------------------------

static M_FORMAT M_DetectFormat(const uint8_t *data, size_t size)
{
    if (size < 4) {
        return M_FMT_UNKNOWN;
    }

    // RIFF....WAVE
    if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F') {
        return M_FMT_WAV;
    }

    // fLaC
    if (data[0] == 'f' && data[1] == 'L' && data[2] == 'a' && data[3] == 'C') {
        return M_FMT_FLAC;
    }

    // OggS
    if (data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S') {
        return M_FMT_OGG;
    }

    // MP3: ID3 tag or sync word 0xFF 0xFB/0xFF 0xF3/0xFF 0xF2
    if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
        return M_FMT_MP3;
    }
    if (size >= 2 && data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) {
        return M_FMT_MP3;
    }

    return M_FMT_UNKNOWN;
}

// ---------------------------------------------------------------------------
// Decoder operations
// ---------------------------------------------------------------------------

static bool M_DecoderOpenMemory(
    M_DECODER *dec, uint8_t *data, size_t size, bool take_ownership)
{
    ASSERT(dec != nullptr);
    ASSERT(data != nullptr);

    memset(dec, 0, sizeof(M_DECODER));
    dec->format = M_DetectFormat(data, size);
    dec->mem_data = take_ownership ? data : nullptr;
    dec->mem_size = size;

    switch (dec->format) {
    case M_FMT_WAV:
        if (!drwav_init_memory(&dec->ctx.wav, data, size, nullptr)) {
            LOG_ERROR("Failed to open WAV from memory");
            return false;
        }
        dec->sample_rate = (int32_t)dec->ctx.wav.sampleRate;
        dec->channels = (int32_t)dec->ctx.wav.channels;
        dec->duration = (double)dec->ctx.wav.totalPCMFrameCount
            / (double)dec->ctx.wav.sampleRate;
        return true;

    case M_FMT_FLAC:
        dec->ctx.flac = drflac_open_memory(data, size, nullptr);
        if (dec->ctx.flac == nullptr) {
            LOG_ERROR("Failed to open FLAC from memory");
            return false;
        }
        dec->sample_rate = (int32_t)dec->ctx.flac->sampleRate;
        dec->channels = (int32_t)dec->ctx.flac->channels;
        dec->duration = (double)dec->ctx.flac->totalPCMFrameCount
            / (double)dec->ctx.flac->sampleRate;
        return true;

    case M_FMT_MP3:
        if (!drmp3_init_memory(&dec->ctx.mp3, data, size, nullptr)) {
            LOG_ERROR("Failed to open MP3 from memory");
            return false;
        }
        dec->sample_rate = (int32_t)dec->ctx.mp3.sampleRate;
        dec->channels = (int32_t)dec->ctx.mp3.channels;
        {
            drmp3_uint64 total = drmp3_get_pcm_frame_count(&dec->ctx.mp3);
            dec->duration = (total > 0)
                ? (double)total / (double)dec->ctx.mp3.sampleRate
                : 0.0;
        }
        return true;

    case M_FMT_OGG: {
        int error = 0;
        dec->ctx.ogg = stb_vorbis_open_memory(data, (int)size, &error, nullptr);
        if (dec->ctx.ogg == nullptr) {
            LOG_ERROR("Failed to open OGG from memory (error %d)", error);
            return false;
        }
        stb_vorbis_info info = stb_vorbis_get_info(dec->ctx.ogg);
        dec->sample_rate = (int32_t)info.sample_rate;
        dec->channels = info.channels;
        dec->duration = stb_vorbis_stream_length_in_seconds(dec->ctx.ogg);
        return true;
    }

    default:
        LOG_ERROR(
            "Unknown audio format (magic: %02X %02X %02X %02X)", data[0],
            data[1], size > 2 ? data[2] : 0, size > 3 ? data[3] : 0);
        return false;
    }
}

static bool M_DecoderOpenFile(M_DECODER *dec, const char *path)
{
    ASSERT(dec != nullptr);
    ASSERT(path != nullptr);

    char *data = nullptr;
    size_t size = 0;
    if (!File_Load(path, &data, &size) || data == nullptr || size == 0) {
        LOG_ERROR("Failed to load audio file: %s", path);
        return false;
    }

    return M_DecoderOpenMemory(dec, (uint8_t *)data, size, true);
}

// Returns number of frames actually read (interleaved float PCM).
// Returns 0 on EOF.
static drwav_uint64 M_DecoderReadFrames(
    M_DECODER *dec, float *buffer, drwav_uint64 frames)
{
    ASSERT(dec != nullptr);

    switch (dec->format) {
    case M_FMT_WAV:
        return drwav_read_pcm_frames_f32(&dec->ctx.wav, frames, buffer);

    case M_FMT_FLAC:
        if (dec->ctx.flac == nullptr) {
            return 0;
        }
        return drflac_read_pcm_frames_f32(dec->ctx.flac, frames, buffer);

    case M_FMT_MP3:
        return drmp3_read_pcm_frames_f32(&dec->ctx.mp3, frames, buffer);

    case M_FMT_OGG: {
        if (dec->ctx.ogg == nullptr) {
            return 0;
        }
        // stb_vorbis_get_samples_float_interleaved returns number of samples
        // (frames) decoded.
        int got = stb_vorbis_get_samples_float_interleaved(
            dec->ctx.ogg, dec->channels, buffer, (int)(frames * dec->channels));
        return (drwav_uint64)got;
    }

    default:
        return 0;
    }
}

static bool M_DecoderSeekToStart(M_DECODER *dec)
{
    ASSERT(dec != nullptr);

    switch (dec->format) {
    case M_FMT_WAV:
        return drwav_seek_to_pcm_frame(&dec->ctx.wav, 0);
    case M_FMT_FLAC:
        return dec->ctx.flac != nullptr
            && drflac_seek_to_pcm_frame(dec->ctx.flac, 0);
    case M_FMT_MP3:
        return drmp3_seek_to_pcm_frame(&dec->ctx.mp3, 0);
    case M_FMT_OGG:
        return dec->ctx.ogg != nullptr && stb_vorbis_seek_start(dec->ctx.ogg);
    default:
        return false;
    }
}

static bool M_DecoderSeekToFrame(M_DECODER *dec, uint64_t frame)
{
    ASSERT(dec != nullptr);

    switch (dec->format) {
    case M_FMT_WAV:
        return drwav_seek_to_pcm_frame(&dec->ctx.wav, frame);
    case M_FMT_FLAC:
        return dec->ctx.flac != nullptr
            && drflac_seek_to_pcm_frame(dec->ctx.flac, frame);
    case M_FMT_MP3:
        return drmp3_seek_to_pcm_frame(&dec->ctx.mp3, frame);
    case M_FMT_OGG:
        return dec->ctx.ogg != nullptr
            && stb_vorbis_seek_frame(dec->ctx.ogg, (unsigned int)frame);
    default:
        return false;
    }
}

static void M_DecoderClose(M_DECODER *dec)
{
    if (dec == nullptr) {
        return;
    }

    switch (dec->format) {
    case M_FMT_WAV:
        drwav_uninit(&dec->ctx.wav);
        break;
    case M_FMT_FLAC:
        if (dec->ctx.flac != nullptr) {
            drflac_close(dec->ctx.flac);
            dec->ctx.flac = nullptr;
        }
        break;
    case M_FMT_MP3:
        drmp3_uninit(&dec->ctx.mp3);
        break;
    case M_FMT_OGG:
        if (dec->ctx.ogg != nullptr) {
            stb_vorbis_close(dec->ctx.ogg);
            dec->ctx.ogg = nullptr;
        }
        break;
    default:
        break;
    }

    Memory_FreePointer(&dec->mem_data);
    dec->format = M_FMT_UNKNOWN;
}

// ---------------------------------------------------------------------------
// Stream helpers
// ---------------------------------------------------------------------------

static void M_Clear(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    stream->is_used = false;
    stream->is_playing = false;
    stream->is_read_done = true;
    stream->is_looped = false;
    stream->volume = 0.0f;
    stream->duration = 0.0;
    stream->played_samples = 0;
    stream->sdl_stream = nullptr;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;

    memset(&stream->decoder, 0, sizeof(stream->decoder));
}

static void M_DiscardSDLStreamData(AUDIO_STREAM_SOUND *const stream)
{
    ASSERT(stream != nullptr);

    if (stream->sdl_stream != nullptr) {
        while (SDL_AudioStreamAvailable(stream->sdl_stream) > 0) {
            const int32_t bytes_gotten = SDL_AudioStreamGet(
                stream->sdl_stream, m_MixBuffer, READ_BUFFER_SIZE);
            if (bytes_gotten <= 0) {
                break;
            }
        }
    }
}

static void M_ResetPlaybackState(
    AUDIO_STREAM_SOUND *const stream, const double relative_timestamp)
{
    ASSERT(stream != nullptr);
    const double clamped = MAX(0.0, relative_timestamp);
    stream->played_samples = (int64_t)(clamped * (double)AUDIO_WORKING_RATE);
}

static void M_SeekToStart(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    M_ResetPlaybackState(stream, 0.0);

    if (stream->start_at > 0.0) {
        uint64_t frame =
            (uint64_t)(stream->start_at * stream->decoder.sample_rate);
        M_DecoderSeekToFrame(&stream->decoder, frame);
    } else {
        M_DecoderSeekToStart(&stream->decoder);
    }

    M_DiscardSDLStreamData(stream);
    stream->is_read_done = false;
}

// Feed decoded PCM into the SDL_AudioStream.  Returns false on EOF.
static bool M_FeedDecoder(AUDIO_STREAM_SOUND *stream)
{
    ASSERT(stream != nullptr);

    if (stream->is_read_done) {
        return false;
    }

    // Check stop_at boundary
    if (stream->stop_at > 0.0) {
        double current_decode_sec =
            (double)stream->played_samples / (double)AUDIO_WORKING_RATE;
        if (current_decode_sec
            >= stream->stop_at - MAX(stream->start_at, 0.0)) {
            if (stream->is_looped) {
                M_SeekToStart(stream);
                return M_FeedDecoder(stream);
            }
            return false;
        }
    }

    drwav_uint64 frames_read =
        M_DecoderReadFrames(&stream->decoder, m_DecodeBuffer, DECODE_FRAMES);

    if (frames_read == 0) {
        if (stream->is_looped) {
            M_SeekToStart(stream);
            return M_FeedDecoder(stream);
        }
        return false;
    }

    int32_t bytes =
        (int32_t)(frames_read * stream->decoder.channels * sizeof(float));
    if (SDL_AudioStreamPut(stream->sdl_stream, m_DecodeBuffer, bytes) != 0) {
        LOG_ERROR("SDL_AudioStreamPut failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

static bool M_InitialiseStream(int32_t sound_id, M_DECODER *dec)
{
    ASSERT(dec != nullptr);

    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];

    stream->decoder = *dec;
    stream->is_read_done = false;
    stream->is_used = true;
    stream->is_playing = true;
    stream->is_looped = false;
    stream->volume = 1.0f;
    stream->played_samples = 0;
    stream->finish_callback = nullptr;
    stream->finish_callback_user_data = nullptr;
    stream->duration = dec->duration;
    stream->start_at = -1.0;
    stream->stop_at = -1.0;

    // Create SDL_AudioStream to resample from source format to working format
    stream->sdl_stream = SDL_NewAudioStream(
        AUDIO_F32SYS, dec->channels, dec->sample_rate, AUDIO_F32SYS,
        AUDIO_WORKING_CHANNELS, AUDIO_WORKING_RATE);
    if (stream->sdl_stream == nullptr) {
        LOG_ERROR("Failed to create SDL_AudioStream: %s", SDL_GetError());
        return false;
    }

    // Pre-fill some data
    M_FeedDecoder(stream);

    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Audio_Stream_Init(void)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        M_Clear(&m_Streams[sound_id]);
    }
}

void Audio_Stream_Shutdown(void)
{
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

        M_DECODER dec;
        if (!M_DecoderOpenFile(&dec, file_path)) {
            LOG_ERROR("Failed to open audio stream: %s", file_path);
            return AUDIO_NO_SOUND;
        }

        Audio_LockDevice();
        bool ok = M_InitialiseStream(sound_id, &dec);
        Audio_UnlockDevice();

        if (!ok) {
            M_DecoderClose(&dec);
            return AUDIO_NO_SOUND;
        }

        return sound_id;
    }

    return AUDIO_NO_SOUND;
}

int32_t Audio_Stream_CreateFromMemory(uint8_t *const data, const size_t size)
{
    if (!g_AudioDeviceID) {
        return AUDIO_NO_SOUND;
    }

    ASSERT(data != nullptr);
    ASSERT(size != 0);

    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];
        if (stream->is_used) {
            continue;
        }

        M_DECODER dec;
        // Memory streams take ownership of the buffer.
        if (!M_DecoderOpenMemory(&dec, data, size, true)) {
            return AUDIO_NO_SOUND;
        }

        Audio_LockDevice();
        bool ok = M_InitialiseStream(sound_id, &dec);
        Audio_UnlockDevice();

        if (!ok) {
            M_DecoderClose(&dec);
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

    Audio_LockDevice();

    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];

    M_DecoderClose(&stream->decoder);

    if (stream->sdl_stream != nullptr) {
        SDL_FreeAudioStream(stream->sdl_stream);
        stream->sdl_stream = nullptr;
    }

    void (*finish_callback)(int32_t, void *) = stream->finish_callback;
    void *finish_callback_user_data = stream->finish_callback_user_data;

    M_Clear(stream);

    Audio_UnlockDevice();

    if (finish_callback != nullptr) {
        finish_callback(sound_id, finish_callback_user_data);
    }

    return true;
}

bool Audio_Stream_Pause(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
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

bool Audio_Stream_Unpause(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
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

bool Audio_Stream_SyncTimestamp(const int32_t sound_id, const double timestamp)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
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

double Audio_Stream_GetTimestamp(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return -1.0;
    }

    double timestamp = -1.0;
    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];

    if (stream->duration > 0.0) {
        Audio_LockDevice();
        timestamp = (double)stream->played_samples / (double)AUDIO_WORKING_RATE;
        Audio_UnlockDevice();
    }

    return timestamp;
}

double Audio_Stream_GetDuration(int32_t sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return -1.0;
    }

    Audio_LockDevice();
    AUDIO_STREAM_SOUND *stream = &m_Streams[sound_id];
    double duration = stream->duration;
    Audio_UnlockDevice();
    return duration;
}

bool Audio_Stream_SeekTimestamp(const int32_t sound_id, const double timestamp)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
    if (!stream->is_used) {
        return false;
    }

    Audio_LockDevice();

    const double abs_time = MAX(stream->start_at, 0.0) + timestamp;
    const uint64_t target_frame =
        (uint64_t)(abs_time * stream->decoder.sample_rate);

    if (!M_DecoderSeekToFrame(&stream->decoder, target_frame)) {
        LOG_ERROR("seek failed for timestamp %f", timestamp);
        Audio_UnlockDevice();
        return false;
    }

    M_DiscardSDLStreamData(stream);
    M_ResetPlaybackState(stream, timestamp);
    stream->is_read_done = false;

    Audio_UnlockDevice();
    return true;
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

void Audio_Stream_Mix(float *dst_buffer, size_t len)
{
    for (int32_t sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS;
         sound_id++) {
        AUDIO_STREAM_SOUND *const stream = &m_Streams[sound_id];
        if (!stream->is_playing) {
            continue;
        }

        // Ensure the SDL_AudioStream has enough resampled data
        while ((SDL_AudioStreamAvailable(stream->sdl_stream) < (int32_t)len)
               && !stream->is_read_done) {
            if (!M_FeedDecoder(stream)) {
                stream->is_read_done = true;
            }
        }

        memset(m_MixBuffer, 0, READ_BUFFER_SIZE);
        int32_t bytes_gotten = SDL_AudioStreamGet(
            stream->sdl_stream, m_MixBuffer, READ_BUFFER_SIZE);
        if (bytes_gotten < 0) {
            LOG_ERROR("Error reading from sdl_stream: %s", SDL_GetError());
            stream->is_playing = false;
            stream->is_used = false;
            stream->is_read_done = true;
        } else if (bytes_gotten == 0) {
            stream->is_playing = false;
            stream->is_used = false;
            stream->is_read_done = true;
        } else {
            int32_t samples_gotten = bytes_gotten
                / (AUDIO_WORKING_CHANNELS * sizeof(AUDIO_WORKING_FORMAT));
            stream->played_samples += (int64_t)samples_gotten;

            const float *src_ptr = &m_MixBuffer[0];
            float *dst_ptr = dst_buffer;

            for (int32_t s = 0; s < samples_gotten; s++) {
                for (int32_t c = 0; c < AUDIO_WORKING_CHANNELS; c++) {
                    *dst_ptr++ += *src_ptr++ * stream->volume;
                }
            }
        }

        if (!stream->is_used) {
            Audio_Stream_Close(sound_id);
        }
    }
}

#endif // EMSCRIPTEN_BUILD
