#define S_AUDIO_IMPL
#include "specific/s_audio.h"

#include "game/shell.h"
#include "log.h"
#include "memory.h"
#include "util.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define MAX_SAMPLES 1000
#define MAX_ACTIVE_SAMPLES 10

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
static AUDIO_SAMPLE m_LoadedSamples[MAX_SAMPLES] = { 0 };
static AUDIO_SAMPLE_SOUND m_SampleSounds[MAX_ACTIVE_SAMPLES] = { 0 };

static double S_Audio_DecibelToMultiplier(double db_gain)
{
    return pow(2.0, db_gain / 600.0);
}

static bool S_Audio_SampleRecalculateChannelVolumes(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0 || sound_id >= MAX_ACTIVE_SAMPLES) {
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

static bool S_Audio_SampleLoad(int sample_id, const char *content, size_t size)
{
    if (!g_AudioDeviceID || sample_id < 0 || sample_id >= MAX_SAMPLES) {
        return false;
    }

    AUDIO_SAMPLE *sample = &m_LoadedSamples[sample_id];

    float *working_buffer = NULL;

    struct {
        size_t read_buffer_size;
        unsigned char *read_buffer;
        AVIOContext *avio_context;
        AVStream *stream;
        AVFormatContext *format_ctx;
        const AVCodec *codec;
        AVCodecContext *codec_ctx;
        AVPacket *packet;
        AVFrame *frame;
    } av = {
        .read_buffer_size = 8192,
        .read_buffer = NULL,
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
        SDL_AudioStream *stream;
    } sdl = {
        .stream = NULL,
    };

    int error_code;

    av.read_buffer = av_malloc(av.read_buffer_size);
    if (!av.read_buffer) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    AUDIO_AV_BUFFER av_buf = {
        .data = content,
        .ptr = content,
        .size = size,
        .remaining = size,
    };

    av.avio_context = avio_alloc_context(
        av.read_buffer, av.read_buffer_size, 0, &av_buf, S_Audio_ReadAVBuffer,
        NULL, NULL);

    av.format_ctx = avformat_alloc_context();
    av.format_ctx->pb = av.avio_context;
    error_code =
        avformat_open_input(&av.format_ctx, "dummy_filename", NULL, NULL);
    if (error_code != 0) {
        goto fail;
    }

    error_code = avformat_find_stream_info(av.format_ctx, NULL);
    if (error_code < 0) {
        goto fail;
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
        goto fail;
    }

    av.codec = avcodec_find_decoder(av.stream->codecpar->codec_id);
    if (!av.codec) {
        error_code = AVERROR_DEMUXER_NOT_FOUND;
        goto fail;
    }

    av.codec_ctx = avcodec_alloc_context3(av.codec);
    if (!av.codec_ctx) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    error_code =
        avcodec_parameters_to_context(av.codec_ctx, av.stream->codecpar);
    if (error_code) {
        goto fail;
    }

    error_code = avcodec_open2(av.codec_ctx, av.codec, NULL);
    if (error_code < 0) {
        goto fail;
    }

    av.packet = av_packet_alloc();
    av_new_packet(av.packet, 0);

    av.frame = av_frame_alloc();
    if (!av.frame) {
        error_code = AVERROR(ENOMEM);
        goto fail;
    }

    size_t num_samples = 0;
    bool first_sample = true;

    while (1) {
        error_code = av_read_frame(av.format_ctx, av.packet);
        if (error_code == AVERROR_EOF) {
            break;
        }

        if (error_code < 0) {
            goto fail;
        }

        error_code = avcodec_send_packet(av.codec_ctx, av.packet);
        if (error_code < 0) {
            goto fail;
        }

        if (first_sample) {
            first_sample = false;

            sdl.src_sample_rate = av.codec_ctx->sample_rate;
            sdl.src_channels = av.codec_ctx->channels;
            switch (av.codec_ctx->sample_fmt) {
            case AV_SAMPLE_FMT_U8:
                sdl.src_format = AUDIO_U8;
                break;

            case AV_SAMPLE_FMT_S16:
                sdl.src_format = AUDIO_S16;
                break;

            case AV_SAMPLE_FMT_S32:
                sdl.src_format = AUDIO_S32;
                break;

            default:
                LOG_ERROR(
                    "Unknown sample format: %d", av.codec_ctx->sample_fmt);
                error_code = AVERROR(ENOTRECOVERABLE);
                goto fail;
            }

            sdl.stream = SDL_NewAudioStream(
                sdl.src_format, sdl.src_channels, sdl.src_sample_rate,
                AUDIO_WORKING_FORMAT, sdl.src_channels, AUDIO_WORKING_RATE);

            if (!sdl.stream) {
                LOG_ERROR("Failed to create SDL stream: %s", SDL_GetError());
                error_code = AVERROR(ENOTRECOVERABLE);
                goto fail;
            }
        }

        while (1) {
            error_code = avcodec_receive_frame(av.codec_ctx, av.frame);

            if (error_code == AVERROR(EAGAIN)) {
                break;
            }

            if (error_code < 0) {
                goto fail;
            }

            error_code = av_samples_get_buffer_size(
                NULL, av.codec_ctx->channels, av.frame->nb_samples,
                av.codec_ctx->sample_fmt, 1);

            if (error_code == AVERROR(EAGAIN)) {
                break;
            }

            if (error_code < 0) {
                goto fail;
            }

            int data_size = error_code;

            if (SDL_AudioStreamPut(sdl.stream, av.frame->data[0], data_size)) {
                LOG_ERROR(
                    "Failed to put data in the SDL stream: %s", SDL_GetError());
                error_code = AVERROR(ENOTRECOVERABLE);
                goto fail;
            }

            num_samples += av.frame->nb_samples;
        }
    }

    SDL_AudioStreamFlush(sdl.stream);

    size_t working_buffer_size = SDL_AudioStreamAvailable(sdl.stream);
    working_buffer = Memory_Alloc(working_buffer_size);
    if (SDL_AudioStreamGet(sdl.stream, working_buffer, working_buffer_size)
        < 0) {
        LOG_ERROR("Failed to get data from the SDL stream: %s", SDL_GetError());
        error_code = AVERROR(ENOTRECOVERABLE);
        goto fail;
    }

    sample->num_samples =
        working_buffer_size / sizeof(AUDIO_WORKING_FORMAT) / sdl.src_channels;
    sample->channels = sdl.src_channels;
    sample->sample_data = working_buffer;

    return true;

fail:
    LOG_ERROR(
        "Error while opening sample ID %d: %s", sample_id,
        av_err2str(error_code));

    if (av.codec_ctx) {
        avcodec_close(av.codec_ctx);
        av_free(av.codec_ctx);
        av.codec_ctx = NULL;
    }

    if (av.format_ctx) {
        avformat_close_input(&av.format_ctx);
        av.format_ctx = NULL;
    }

    if (av.packet) {
        av_packet_free(&av.packet);
        av.packet = NULL;
    }

    if (av.frame) {
        av_frame_free(&av.frame);
        av.frame = NULL;
    }

    av.codec = NULL;

    sample->sample_data = NULL;
    sample->num_samples = 0;
    sample->channels = 0;

    if (working_buffer) {
        Memory_Free(working_buffer);
        working_buffer = NULL;
    }

    if (av.read_buffer) {
        av_free(av.read_buffer);
        av.read_buffer = NULL;
    }

    if (av.avio_context) {
        av_free(av.avio_context);
        av.avio_context = NULL;
    }

    if (sdl.stream) {
        SDL_FreeAudioStream(sdl.stream);
        sdl.stream = NULL;
    }

    return false;
}

void S_Audio_SampleSoundInit()
{
    for (int sound_id = 0; sound_id < MAX_ACTIVE_SAMPLES; sound_id++) {
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

void S_Audio_SampleSoundShutdown()
{
    if (!g_AudioDeviceID) {
        return;
    }

    S_Audio_SampleSoundCloseAll();
}

bool S_Audio_SamplesClear()
{
    if (!g_AudioDeviceID) {
        return false;
    }

    S_Audio_SampleSoundCloseAll();

    for (int i = 0; i < MAX_ACTIVE_SAMPLES; i++) {
        if (m_LoadedSamples[i].sample_data) {
            Memory_Free(m_LoadedSamples[i].sample_data);
            m_LoadedSamples[i].sample_data = NULL;
        }
    }

    return true;
}

bool S_Audio_SamplesLoad(size_t count, const char **contents, size_t *sizes)
{
    if (!g_AudioDeviceID) {
        return false;
    }

    if (count > MAX_SAMPLES) {
        Shell_ExitSystemFmt(
            "Trying to load too many samples (maximum supported count: %d)",
            MAX_SAMPLES);
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
    for (int sound_id = 0; sound_id < MAX_ACTIVE_SAMPLES; sound_id++) {
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

    return result;
}

bool S_Audio_SampleSoundIsPlaying(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0 || sound_id >= MAX_ACTIVE_SAMPLES) {
        return false;
    }

    return m_SampleSounds[sound_id].is_playing;
}

bool S_Audio_SampleSoundClose(int sound_id)
{
    if (!g_AudioDeviceID || sound_id < 0 || sound_id >= MAX_ACTIVE_SAMPLES) {
        return false;
    }

    SDL_LockAudioDevice(g_AudioDeviceID);
    m_SampleSounds[sound_id].is_used = false;
    m_SampleSounds[sound_id].is_playing = false;
    SDL_UnlockAudioDevice(g_AudioDeviceID);

    return true;
}

bool S_Audio_SampleSoundCloseAll()
{
    if (!g_AudioDeviceID) {
        return false;
    }

    for (int sound_id = 0; sound_id < MAX_ACTIVE_SAMPLES; sound_id++) {
        if (m_SampleSounds[sound_id].is_used) {
            S_Audio_SampleSoundClose(sound_id);
        }
    }

    return true;
}

bool S_Audio_SampleSoundSetPan(int sound_id, int pan)
{
    if (!g_AudioDeviceID || sound_id < 0 || sound_id >= MAX_ACTIVE_SAMPLES) {
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
    if (!g_AudioDeviceID || sound_id < 0 || sound_id >= MAX_ACTIVE_SAMPLES) {
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
    for (int sound_id = 0; sound_id < MAX_ACTIVE_SAMPLES; sound_id++) {
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
                    sound->is_playing = false;
                    sound->is_used = false;
                    break;
                }
            }
        }

        sound->current_sample = src_sample_idx;

        if (!sound->is_used) {
            S_Audio_SampleSoundClose(sound_id);
        }
    }
}
