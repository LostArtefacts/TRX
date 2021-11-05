#include "specific/sound.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "specific/init.h"

#include <math.h>

#define DECIBEL_LUT_SIZE 512

typedef struct SAMPLE_DATA {
    char *data;
    int32_t length;
    int16_t bits_per_sample;
    int16_t channels;
    int16_t sample_rate;
    int16_t block_align;
    int16_t volume;
    int32_t pan;
    void *handle;
} SAMPLE_DATA;

typedef struct DUPE_SOUND_BUFFER {
    SAMPLE_DATA *sample;
    LPDIRECTSOUNDBUFFER buffer;
    struct DUPE_SOUND_BUFFER *next;
} DUPE_SOUND_BUFFER;

#pragma pack(push, 1)
typedef struct WAVE_FORMAT_CHUNK {
    char subchunk_id[4];
    int32_t subchunk_size;
    int16_t audio_format;
    int16_t num_channels;
    int32_t sample_rate;
    int32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;
} WAVE_FORMAT_CHUNK;

typedef struct WAVE_DATA_CHUNK {
    char subchunk_id[4];
    int32_t subchunk_size;
    // data
} WAVE_DATA_CHUNK;

typedef struct WAVE_FILE_HEADER {
    char chunk_id[4];
    int32_t chunk_size;
    char format[4];
    WAVE_FORMAT_CHUNK fmt_chunk;
    WAVE_DATA_CHUNK data_chunk;
} WAVE_FILE_HEADER;
#pragma pack(pop)

static DUPE_SOUND_BUFFER *DupeSoundBufferList = NULL;
static int32_t DecibelLUT[DECIBEL_LUT_SIZE] = { 0 };
extern int32_t NumSampleData;
static SAMPLE_DATA **SampleData = NULL;

static int32_t ConvertVolumeToDecibel(int32_t volume);
static int32_t ConvertPanToDecibel(uint16_t pan);
static SAMPLE_DATA *S_Sound_LoadSample(char *content);
static bool S_Sound_MakeSample(SAMPLE_DATA *sample_data);

static int32_t ConvertVolumeToDecibel(int32_t volume)
{
    return DecibelLUT[(volume & 0x7FFF) >> 6];
}

static int32_t ConvertPanToDecibel(uint16_t pan)
{
    int32_t result = sin((pan / 32767.0) * M_PI) * (DECIBEL_LUT_SIZE / 2);
    if (result > 0) {
        return -DecibelLUT[DECIBEL_LUT_SIZE - result];
    } else if (result < 0) {
        return DecibelLUT[DECIBEL_LUT_SIZE + result];
    } else {
        return 0;
    }
}

void *S_Sound_PlaySample(
    int32_t sample_id, int32_t volume, int16_t pitch, uint16_t pan, bool loop)
{
    volume = (Sound_MasterVolume * volume) >> 6;

    if (!SoundIsActive) {
        return NULL;
    }

    SAMPLE_DATA *sample = SampleData[sample_id];
    if (!sample) {
        return NULL;
    }

    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)sample->handle;

    // check if the buffer is already playing
    DWORD status;
    HRESULT result = IDirectSoundBuffer_GetStatus(
        (LPDIRECTSOUNDBUFFER)sample->handle, &status);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_GetStatus: 0x%lx", result);
        return NULL;
    }

    if (status == DSBSTATUS_PLAYING) {
        buffer = NULL;

        for (DUPE_SOUND_BUFFER *dupe_buffer = DupeSoundBufferList;
             dupe_buffer != NULL; dupe_buffer = dupe_buffer->next) {
            if (dupe_buffer->sample == sample) {
                result =
                    IDirectSoundBuffer_GetStatus(dupe_buffer->buffer, &status);
                if (result != DS_OK) {
                    LOG_ERROR(
                        "Error while calling IDirectSoundBuffer_GetStatus: "
                        "0x%lx",
                        result);
                    continue;
                }
                if (status != DSBSTATUS_PLAYING) {
                    buffer = dupe_buffer->buffer;
                    break;
                }
            }
        }

        if (!buffer) {
            LPDIRECTSOUNDBUFFER buffer_new;
            result = IDirectSound8_DuplicateSoundBuffer(
                DSound, (LPDIRECTSOUNDBUFFER)sample->handle, &buffer_new);
            if (result != DS_OK) {
                LOG_ERROR(
                    "Error while calling IDirectSound8_DuplicateSoundBuffer: "
                    "0x%lx",
                    result);
                return NULL;
            }

            DUPE_SOUND_BUFFER *dupe_buffer = malloc(sizeof(DUPE_SOUND_BUFFER));
            dupe_buffer->buffer = buffer_new;
            dupe_buffer->next = DupeSoundBufferList;
            DupeSoundBufferList = dupe_buffer;

            buffer = buffer_new;
            LOG_DEBUG(
                "duplicated sound buffer %p to %p", sample->handle, buffer_new);
        }
    }

    // calculate pitch from sample rate
    int32_t ds_pitch = sample->sample_rate;
    if (pitch != -1) {
        ds_pitch = ds_pitch * pitch / 100;
    }

    result = IDirectSoundBuffer_SetFrequency(buffer, ds_pitch);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_SetFrequency: 0x%lx",
            result);
        return NULL;
    }
    result = IDirectSoundBuffer_SetPan(buffer, ConvertPanToDecibel(pan));
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_SetPan: 0x%lx", result);
        return NULL;
    }
    result =
        IDirectSoundBuffer_SetVolume(buffer, ConvertVolumeToDecibel(volume));
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_SetVolume: 0x%lx", result);
        return NULL;
    }
    result = IDirectSoundBuffer_SetCurrentPosition(buffer, 0);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_SetCurrentPosition: 0x%lx",
            result);
        return NULL;
    }
    result = IDirectSoundBuffer_Play(buffer, 0, 0, loop ? DSBPLAY_LOOPING : 0);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling IDirectSoundBuffer_Play: 0x%lx", result);
        return NULL;
    }
    return buffer;
}

bool S_Sound_Init()
{
    HRESULT result = DirectSoundCreate(0, &DSound, 0);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling DirectSoundCreate: 0x%lx", result);
        return false;
    }
    result = DSound->lpVtbl->SetCooperativeLevel(DSound, TombHWND, 1);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling SetCooperativeLevel: 0x%lx", result);
        return false;
    }
    DecibelLUT[0] = -10000;
    for (int i = 1; i < DECIBEL_LUT_SIZE; i++) {
        DecibelLUT[i] = -9000.0 - log2(1.0 / i) * -1000.0 / log2(0.5);
    }
    return true;
}

void S_Sound_StopAllSamples()
{
    if (!SoundIsActive) {
        return;
    }

    for (int i = 0; i < NumSampleData; i++) {
        SAMPLE_DATA *sample = SampleData[i];
        if (sample) {
            S_Sound_StopSample(sample->handle);
        }
    }
}

void S_Sound_LoadSamples(char **sample_pointers, int32_t num_samples)
{
    if (!SoundIsActive) {
        return;
    }

    NumSampleData = num_samples;
    SampleData = malloc(sizeof(SAMPLE_DATA *) * num_samples);
    for (int i = 0; i < NumSampleData; i++) {
        SampleData[i] = S_Sound_LoadSample(sample_pointers[i]);
    }
}

static SAMPLE_DATA *S_Sound_LoadSample(char *content)
{
    WAVE_FILE_HEADER *hdr = (WAVE_FILE_HEADER *)content;
    if (strncmp(hdr->chunk_id, "RIFF", 4)) {
        S_ExitSystem("Samples must be in WAVE format.");
        return NULL;
    }

    SAMPLE_DATA *sample_data = malloc(sizeof(SAMPLE_DATA));
    memset(sample_data, 0, sizeof(SAMPLE_DATA));
    sample_data->data = content + sizeof(WAVE_FILE_HEADER);
    sample_data->length =
        hdr->data_chunk.subchunk_size - sizeof(WAVE_FILE_HEADER);
    sample_data->bits_per_sample = hdr->fmt_chunk.bits_per_sample;
    sample_data->channels = hdr->fmt_chunk.num_channels;
    sample_data->block_align =
        sample_data->channels * hdr->fmt_chunk.bits_per_sample / 8;
    sample_data->sample_rate = hdr->fmt_chunk.sample_rate;

    sample_data->pan = 0;
    sample_data->volume = 0x7FFF;
    if (S_Sound_MakeSample(sample_data)) {
        return sample_data;
    }
    return NULL;
}

static bool S_Sound_MakeSample(SAMPLE_DATA *sample_data)
{
    WAVEFORMATEX wave_format;
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = sample_data->channels;
    wave_format.nSamplesPerSec = sample_data->sample_rate;
    wave_format.nAvgBytesPerSec =
        sample_data->sample_rate * sample_data->block_align;
    wave_format.nBlockAlign = sample_data->block_align;
    wave_format.wBitsPerSample = sample_data->bits_per_sample;

    DSBUFFERDESC buffer_desc;
    buffer_desc.dwSize = sizeof(DSBUFFERDESC);
    buffer_desc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN
        | DSBCAPS_CTRLFREQUENCY;
    buffer_desc.dwBufferBytes = sample_data->length;
    buffer_desc.dwReserved = 0;
    buffer_desc.lpwfxFormat = &wave_format;
    CLAMP(buffer_desc.dwBufferBytes, DSBSIZE_MIN, DSBSIZE_MAX);

    HRESULT result = IDirectSound_CreateSoundBuffer(
        DSound, &buffer_desc, (LPLPDIRECTSOUNDBUFFER)&sample_data->handle, 0);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSound_CreateSoundBuffer: 0x%lx",
            result);
        S_ExitSystem("Fatal DirectSound error!");
    }

    DWORD audio_data_size;
    LPVOID audio_data;

    result = IDirectSoundBuffer_Lock(
        (LPDIRECTSOUNDBUFFER)sample_data->handle, 0, buffer_desc.dwBufferBytes,
        &audio_data, &audio_data_size, 0, 0, 0);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling IDirectSoundBuffer_Lock: 0x%lx", result);
        S_ExitSystem("Fatal DirectSound error!");
    }

    memcpy(audio_data, sample_data->data, buffer_desc.dwBufferBytes);

    result = IDirectSoundBuffer_Unlock(
        (LPDIRECTSOUNDBUFFER)sample_data->handle, audio_data, audio_data_size,
        0, 0);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_Unlock: 0x%lx", result);
        S_ExitSystem("Fatal DirectSound error!");
    }

    return true;
}

void S_Sound_StopSample(void *handle)
{
    if (!SoundIsActive) {
        return;
    }
    if (!handle) {
        return;
    }
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    HRESULT result = IDirectSoundBuffer_Stop(buffer);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling IDirectSoundBuffer_Stop: 0x%lx", result);
    }
}

void S_Sound_SetPanAndVolume(void *handle, int16_t pan, int16_t volume)
{
    if (!SoundIsActive) {
        return;
    }
    if (!handle) {
        return;
    }
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    HRESULT result;
    result = IDirectSoundBuffer_SetVolume(
        buffer, ConvertVolumeToDecibel((Sound_MasterVolume * volume) >> 6));
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_SetVolume: 0x%lx", result);
    }
    result = IDirectSoundBuffer_SetPan(buffer, ConvertPanToDecibel(pan));
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_SetPan: 0x%lx", result);
    }
}

bool S_Sound_SampleIsPlaying(void *handle)
{
    if (!SoundIsActive) {
        return false;
    }
    if (!handle) {
        return false;
    }
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    DWORD status;
    HRESULT result = IDirectSoundBuffer_GetStatus(buffer, &status);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_GetStatus: 0x%lx", result);
        return false;
    }
    return status == DSBSTATUS_PLAYING;
}
