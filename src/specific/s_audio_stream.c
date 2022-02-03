#define S_AUDIO_IMPL
#include "specific/s_audio.h"

#include "memory.h"
#include "filesystem.h"
#include "log.h"

#include <assert.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define READ_BUFFER_SIZE                                                       \
    (AUDIO_SAMPLES * AUDIO_WORKING_CHANNELS * sizeof(AUDIO_WORKING_FORMAT))

typedef struct AUDIO_STREAM_SOUND {
    bool is_used;
    bool is_playing;
    bool is_read_done;
    bool is_looped;
    float volume;

    void (*finish_callback)(int sound_id, void *user_data);
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
        SDL_AudioStream *stream;
    } sdl;
} AUDIO_STREAM_SOUND;

extern SDL_AudioDeviceID g_AudioDeviceID;

static AUDIO_STREAM_SOUND m_StreamSounds[AUDIO_MAX_ACTIVE_STREAMS] = { 0 };
static float m_DecodeBuffer[AUDIO_SAMPLES * AUDIO_WORKING_CHANNELS] = { 0 };

static bool S_Audio_StreamSoundDecodeFrame(AUDIO_STREAM_SOUND *stream);
static bool S_Audio_StreamSoundEnqueueFrame(AUDIO_STREAM_SOUND *stream);
static bool S_Audio_StreamSoundInitialiseFromPath(
    int sound_id, const char *file_path);

static bool S_Audio_StreamSoundDecodeFrame(AUDIO_STREAM_SOUND *stream)
{
    assert(stream);

    int error_code = av_read_frame(stream->av.format_ctx, stream->av.packet);

    if (error_code == AVERROR_EOF && stream->is_looped) {
        avio_seek(stream->av.format_ctx->pb, 0, SEEK_SET);
        avformat_seek_file(
            stream->av.format_ctx, -1, 0, 0, 0, AVSEEK_FLAG_FRAME);
        return S_Audio_StreamSoundDecodeFrame(stream);
    }

    if (error_code < 0) {
        return false;
    }

    if (stream->av.packet->stream_index != stream->av.stream->index) {
        return true;
    }

    error_code = avcodec_send_packet(stream->av.codec_ctx, stream->av.packet);
    if (error_code < 0) {
        LOG_ERROR(
            "Got an error when decoding frame: %s", av_err2str(error_code));
        return false;
    }

    return true;
}

static bool S_Audio_StreamSoundEnqueueFrame(AUDIO_STREAM_SOUND *stream)
{
    assert(stream);

    while (1) {
        int error_code =
            avcodec_receive_frame(stream->av.codec_ctx, stream->av.frame);
        if (error_code == AVERROR(EAGAIN)) {
            break;
        }

        if (error_code < 0) {
            LOG_ERROR(
                "Got an error when decoding frame: %d, %s", error_code,
                av_err2str(error_code));
            break;
        }

        error_code = av_samples_get_buffer_size(
            NULL, stream->av.codec_ctx->channels, stream->av.frame->nb_samples,
            stream->av.codec_ctx->sample_fmt, 1);

        if (error_code == AVERROR(EAGAIN)) {
            break;
        }

        if (error_code < 0) {
            LOG_ERROR(
                "Got an error when decoding frame: %d, %s", error_code,
                av_err2str(error_code));
            break;
        }

        int data_size = error_code;

        if (SDL_AudioStreamPut(
                stream->sdl.stream, stream->av.frame->data[0], data_size)) {
            LOG_ERROR("Got an error when decoding frame: %s", SDL_GetError());
            break;
        }
    }

    return true;
}

static bool S_Audio_StreamSoundInitialiseFromPath(
    int sound_id, const char *file_path)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    bool ret = false;
    SDL_LockAudioDevice(g_AudioDeviceID);

    int error_code;
    char *full_path = NULL;
    File_GetFullPath(file_path, &full_path);

    AUDIO_STREAM_SOUND *stream = &m_StreamSounds[sound_id];

    error_code =
        avformat_open_input(&stream->av.format_ctx, full_path, NULL, NULL);
    if (error_code != 0) {
        goto cleanup;
    }

    error_code = avformat_find_stream_info(stream->av.format_ctx, NULL);
    if (error_code < 0) {
        goto cleanup;
    }

    stream->av.stream = NULL;
    for (unsigned int i = 0; i < stream->av.format_ctx->nb_streams; i++) {
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

    error_code = avcodec_open2(stream->av.codec_ctx, stream->av.codec, NULL);
    if (error_code < 0) {
        goto cleanup;
    }

    stream->av.packet = av_packet_alloc();
    if (!stream->av.packet) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }
    av_new_packet(stream->av.packet, 0);

    stream->av.frame = av_frame_alloc();
    if (!stream->av.frame) {
        error_code = AVERROR(ENOMEM);
        goto cleanup;
    }

    S_Audio_StreamSoundDecodeFrame(stream);

    int32_t sdl_format =
        S_Audio_GetSDLAudioFormat(stream->av.codec_ctx->sample_fmt);
    if (sdl_format < 0) {
        LOG_ERROR(
            "Unknown sample format: %d", stream->av.codec_ctx->sample_fmt);
        goto cleanup;
    }

    int32_t sdl_sample_rate = stream->av.codec_ctx->sample_rate;
    int32_t sdl_channels = stream->av.codec_ctx->channels;

    stream->is_read_done = false;
    stream->is_used = true;
    stream->is_playing = true;
    stream->is_looped = false;
    stream->volume = 1.0f;
    stream->finish_callback = NULL;

    stream->sdl.stream = SDL_NewAudioStream(
        sdl_format, sdl_channels, sdl_sample_rate, AUDIO_WORKING_FORMAT,
        sdl_channels, AUDIO_WORKING_RATE);
    if (!stream->sdl.stream) {
        LOG_ERROR("Failed to create SDL stream: %s", SDL_GetError());
        goto cleanup;
    }

    ret = true;
    S_Audio_StreamSoundEnqueueFrame(stream);

cleanup:
    if (error_code) {
        LOG_ERROR(
            "Error while opening audio %s: %s", file_path,
            av_err2str(error_code));
    }

    if (!ret) {
        S_Audio_StreamSoundClose(sound_id);
    }

    SDL_UnlockAudioDevice(g_AudioDeviceID);
    Memory_FreePointer(&full_path);
    return ret;
}

void S_Audio_StreamSoundInit()
{
    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS; sound_id++) {
        AUDIO_STREAM_SOUND *stream = &m_StreamSounds[sound_id];
        stream->is_used = false;
        stream->is_playing = false;
        stream->is_read_done = true;
        stream->volume = 0.0f;
        stream->sdl.stream = NULL;
        stream->finish_callback = NULL;
    }
}

void S_Audio_StreamSoundShutdown()
{
    if (!g_AudioDeviceID) {
        return;
    }

    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS; sound_id++) {
        if (m_StreamSounds[sound_id].is_used) {
            S_Audio_StreamSoundClose(sound_id);
        }
    }
}

bool S_Audio_StreamSoundPause(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    if (m_StreamSounds[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_StreamSounds[sound_id].is_playing = false;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return true;
}

bool S_Audio_StreamSoundUnpause(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    if (!m_StreamSounds[sound_id].is_playing) {
        SDL_LockAudioDevice(g_AudioDeviceID);
        m_StreamSounds[sound_id].is_playing = true;
        SDL_UnlockAudioDevice(g_AudioDeviceID);
    }

    return true;
}

int S_Audio_StreamSoundCreateFromFile(const char *file_path)
{
    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS; sound_id++) {
        AUDIO_STREAM_SOUND *stream = &m_StreamSounds[sound_id];
        if (stream->is_used) {
            continue;
        }

        if (!S_Audio_StreamSoundInitialiseFromPath(sound_id, file_path)) {
            return AUDIO_NO_SOUND;
        }

        return sound_id;
    }

    return AUDIO_NO_SOUND;
}

bool S_Audio_StreamSoundClose(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);

    AUDIO_STREAM_SOUND *stream = &m_StreamSounds[sound_id];

    if (stream->av.codec_ctx) {
        avcodec_close(stream->av.codec_ctx);
        av_free(stream->av.codec_ctx);
        stream->av.codec_ctx = NULL;
    }

    if (stream->av.format_ctx) {
        avformat_close_input(&stream->av.format_ctx);
        stream->av.format_ctx = NULL;
    }

    if (stream->av.packet) {
        av_packet_free(&stream->av.packet);
        stream->av.packet = NULL;
    }

    if (stream->av.frame) {
        av_frame_free(&stream->av.frame);
        stream->av.frame = NULL;
    }

    stream->av.stream = NULL;
    stream->av.codec = NULL;

    if (stream->sdl.stream) {
        SDL_FreeAudioStream(stream->sdl.stream);
        stream->sdl.stream = NULL;
    }

    stream->is_read_done = true;
    stream->is_used = false;
    stream->is_playing = false;
    stream->is_looped = false;
    stream->volume = 0.0f;
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    if (stream->finish_callback) {
        stream->finish_callback(sound_id, stream->finish_callback_user_data);
    }

    return true;
}

bool S_Audio_StreamSoundSetVolume(int sound_id, float volume)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_StreamSounds[sound_id].volume = volume;

    return true;
}

bool S_Audio_StreamSoundIsLooped(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    return m_StreamSounds[sound_id].is_looped;
}

bool S_Audio_StreamSoundSetIsLooped(int sound_id, bool is_looped)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_StreamSounds[sound_id].is_looped = is_looped;

    return true;
}

bool S_Audio_StreamSoundSetFinishCallback(
    int sound_id, void (*callback)(int sound_id, void *user_data),
    void *user_data)
{
    if (!g_AudioDeviceID || sound_id < 0
        || sound_id >= AUDIO_MAX_ACTIVE_STREAMS) {
        return false;
    }

    m_StreamSounds[sound_id].finish_callback = callback;
    m_StreamSounds[sound_id].finish_callback_user_data = user_data;

    return true;
}

void S_Audio_StreamSoundMix(float *dst_buffer, size_t len)
{
    for (int sound_id = 0; sound_id < AUDIO_MAX_ACTIVE_STREAMS; sound_id++) {
        AUDIO_STREAM_SOUND *stream = &m_StreamSounds[sound_id];
        if (!stream->is_playing) {
            continue;
        }

        while ((SDL_AudioStreamAvailable(stream->sdl.stream) < (int)len)
               && !stream->is_read_done) {
            if (S_Audio_StreamSoundDecodeFrame(stream)) {
                S_Audio_StreamSoundEnqueueFrame(stream);
            } else {
                stream->is_read_done = true;
            }
        }

        memset(m_DecodeBuffer, 0, READ_BUFFER_SIZE);
        int bytes_gotten = SDL_AudioStreamGet(
            stream->sdl.stream, m_DecodeBuffer, READ_BUFFER_SIZE);
        if (bytes_gotten < 0) {
            LOG_ERROR("Error reading from sdl.stream: %s", SDL_GetError());
            stream->is_playing = false;
            stream->is_used = false;
            stream->is_read_done = true;
        } else if (bytes_gotten == 0) {
            // legit end of stream. looping is handled in
            // S_Audio_StreamSoundDecodeFrame
            stream->is_playing = false;
            stream->is_used = false;
            stream->is_read_done = true;
        } else {
            int samples_gotten = bytes_gotten
                / (stream->av.codec_ctx->channels
                   * sizeof(AUDIO_WORKING_FORMAT));

            const float *src_ptr = &m_DecodeBuffer[0];
            float *dst_ptr = dst_buffer;

            if (stream->av.codec_ctx->channels == 2) {
                for (int s = 0; s < samples_gotten; s++) {
                    *dst_ptr++ += *src_ptr++ * stream->volume;
                    *dst_ptr++ += *src_ptr++ * stream->volume;
                }
            } else if (stream->av.codec_ctx->channels == 1) {
                for (int s = 0; s < samples_gotten; s++) {
                    *dst_ptr++ += *src_ptr * stream->volume;
                    *dst_ptr++ += *src_ptr++ * stream->volume;
                }
            } else {
                for (int s = 0; s < samples_gotten; s++) {
                    // downmix to mono
                    float src_sample = 0.0f;
                    for (int i = 0; i < stream->av.codec_ctx->channels; i++) {
                        src_sample += *src_ptr++;
                    }
                    src_sample /= (float)stream->av.codec_ctx->channels;
                    *dst_ptr++ += src_sample * stream->volume;
                    *dst_ptr++ += src_sample * stream->volume;
                }
            }
        }

        if (!stream->is_used) {
            S_Audio_StreamSoundClose(sound_id);
        }
    }
}
