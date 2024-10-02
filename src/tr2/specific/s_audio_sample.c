#include "specific/s_audio_sample.h"

#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"
#include "lib/dsound.h"
#include "specific/s_flagged_string.h"

#include <libtrx/engine/audio.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct __unaligned
{
    char chunk_id[4];
    uint32_t chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
}
WAVE_FMT_CHUNK;

typedef struct {
    char chunk_id[4];
    uint32_t chunk_size;
} WAVE_DATA_CHUNK;

typedef struct {
    char chunk_id[4];
    uint32_t chunk_size;
} WAVE_RIFF_CHUNK;

typedef struct __unaligned
{
    char format[4];
    WAVE_FMT_CHUNK fmt;
    WAVE_DATA_CHUNK data;
}
WAVE_MAIN;

typedef struct __unaligned
{
    WAVE_RIFF_CHUNK riff;
    WAVE_MAIN main;
}
WAVE_FILE_HEADER;

static void M_CreateWAV(
    const LPWAVEFORMATEX format, const void *data, size_t data_size,
    char **buffer, size_t *buffer_size);

static void M_CreateWAV(
    const LPWAVEFORMATEX format, const void *const data, const size_t data_size,
    char **const buffer, size_t *const buffer_size)
{

    const WAVE_FILE_HEADER header = {
        .riff = {
            .chunk_id = {'R', 'I', 'F', 'F'},
            .chunk_size = sizeof(WAVE_MAIN) + data_size,
        },
        .main = {
            .format = {'W', 'A', 'V', 'E'},
            .fmt = {
                .chunk_id = {'f', 'm', 't', ' '},
                .chunk_size = 16,
                .audio_format = format->wFormatTag,
                .num_channels = format->nChannels,
                .sample_rate = format->nSamplesPerSec,
                .byte_rate = format->nAvgBytesPerSec,
                .block_align = format->nBlockAlign,
                .bits_per_sample = format->wBitsPerSample
            },
            .data = {
                .chunk_id = {'d', 'a', 't', 'a'},
                .chunk_size = data_size,
            }
        }
    };

    *buffer_size = sizeof(header) + data_size;
    *buffer = Memory_Alloc(*buffer_size);

    memcpy(*buffer, &header, sizeof(header));
    memcpy(*buffer + sizeof(header), data, data_size);
}

const SOUND_ADAPTER_NODE *__cdecl S_Audio_Sample_GetAdapter(const GUID *guid)
{
    return NULL;
}

void __cdecl S_Audio_Sample_CloseAllTracks(void)
{
    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();
}

bool __cdecl S_Audio_Sample_Load(
    int32_t sample_id, LPWAVEFORMATEX format, const void *data,
    uint32_t data_size)
{
    char *wave = NULL;
    size_t wave_size;
    M_CreateWAV(format, data, data_size, &wave, &wave_size);

    const bool result = Audio_Sample_LoadSingle(sample_id, wave, wave_size);
    Memory_FreePointer(&wave);
    return result;
}

bool __cdecl S_Audio_Sample_IsTrackPlaying(int32_t track_id)
{
    return false;
}

int32_t __cdecl S_Audio_Sample_Play(
    int32_t sample_id, int32_t volume, int32_t pitch, int32_t pan,
    uint32_t flags)
{
    return -1;
}

int32_t __cdecl S_Audio_Sample_GetFreeTrackIndex(void)
{
    return -1;
}

void __cdecl S_Audio_Sample_AdjustTrackVolumeAndPan(
    int32_t track_id, int32_t volume, int32_t pan)
{
}

void __cdecl S_Audio_Sample_AdjustTrackPitch(int32_t track_id, int32_t pitch)
{
}

void __cdecl S_Audio_Sample_CloseTrack(int32_t track_id)
{
}

bool __cdecl S_Audio_Sample_Init(void)
{
    return false;
}

bool __cdecl S_Audio_Sample_DSoundEnumerate(SOUND_ADAPTER_LIST *adapter_list)
{
    return false;
}

BOOL CALLBACK S_Audio_Sample_DSoundEnumCallback(
    LPGUID guid, LPCTSTR description, LPCTSTR module, LPVOID context)
{
    return TRUE;
}

void __cdecl S_Audio_Sample_Init2(HWND hwnd)
{
}

bool __cdecl S_Audio_Sample_DSoundCreate(GUID *guid)
{
    return false;
}

bool __cdecl S_Audio_Sample_DSoundBufferTest(void)
{
    return false;
}

void __cdecl S_Audio_Sample_Shutdown(void)
{
}

bool __cdecl S_Audio_Sample_IsEnabled(void)
{
    return true;
}

int32_t __cdecl S_Audio_Sample_OutPlay(
    int32_t sample_id, int32_t volume, int32_t pitch, int32_t pan)
{
    return -1;
}

int32_t __cdecl S_Audio_Sample_CalculateSampleVolume(int32_t volume)
{
    return 0;
}

int32_t __cdecl S_Audio_Sample_CalculateSamplePan(int16_t pan)
{
    return 0;
}

int32_t __cdecl S_Audio_Sample_OutPlayLooped(
    int32_t sample_id, int32_t volume, int32_t pitch, int32_t pan)
{
    return -1;
}

void __cdecl S_Audio_Sample_OutSetPanAndVolume(
    int32_t track_id, int32_t pan, int32_t volume)
{
}

void __cdecl S_Audio_Sample_OutSetPitch(int32_t track_id, int32_t pitch)
{
}

void __cdecl S_Audio_Sample_OutCloseTrack(int32_t track_id)
{
}

void __cdecl S_Audio_Sample_OutCloseAllTracks(void)
{
}

bool __cdecl S_Audio_Sample_OutIsTrackPlaying(int32_t track_id)
{
    return false;
}
