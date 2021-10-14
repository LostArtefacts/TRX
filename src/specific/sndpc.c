#include "specific/sndpc.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/init.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>

#define DECIBEL_LUT_SIZE 512

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

int32_t ConvertVolumeToDecibel(int32_t volume)
{
    return DecibelLUT[(volume & 0x7FFF) >> 6];
}

int32_t ConvertPanToDecibel(uint16_t pan)
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

void *SoundPlaySample(
    int32_t sample_id, int32_t volume, int16_t pitch, uint16_t pan, int8_t loop)
{
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

int32_t SoundInit()
{
    HRESULT result = DirectSoundCreate(0, &DSound, 0);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling DirectSoundCreate: 0x%lx", result);
        return 0;
    }
    result = DSound->lpVtbl->SetCooperativeLevel(DSound, TombHWND, 1);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling SetCooperativeLevel: 0x%lx", result);
        return 0;
    }
    DecibelLUT[0] = -10000;
    for (int i = 1; i < DECIBEL_LUT_SIZE; i++) {
        DecibelLUT[i] = -9000.0 - log2(1.0 / i) * -1000.0 / log2(0.5);
    }
    SoundInit1 = 1;
    SoundInit2 = 1;
    return 1;
}

int32_t MusicInit()
{
    MCI_OPEN_PARMS open_parms;
    open_parms.dwCallback = 0;
    open_parms.wDeviceID = 0;
    open_parms.lpstrDeviceType = (LPSTR)MCI_DEVTYPE_CD_AUDIO;
    open_parms.lpstrElementName = NULL;
    open_parms.lpstrAlias = NULL;

    MCIERROR result = mciSendCommandA(
        0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD_PTR)&open_parms);
    if (result) {
        LOG_ERROR("cannot initailize music device: %x", result);
        return 0;
    }
    MCIDeviceID = open_parms.wDeviceID;

    AuxDeviceID = 0;
    for (int i = 0; i < auxGetNumDevs(); i++) {
        AUXCAPSA caps;
        auxGetDevCapsA((UINT_PTR)i, &caps, sizeof(AUXCAPSA));
        if (caps.wTechnology == AUXCAPS_CDAUDIO) {
            AuxDeviceID = i;
        }
    }

    MCI_STATUS_PARMS status_parms;
    status_parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    mciSendCommandA(
        MCIDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&status_parms);
    CDNumTracks = status_parms.dwReturn;
    return 1;
}

void SoundLoadSamples(char **sample_pointers, int32_t num_samples)
{
    if (!SoundIsActive) {
        return;
    }

    NumSampleData = num_samples;
    SampleData = malloc(sizeof(SAMPLE_DATA *) * num_samples);
    for (int i = 0; i < NumSampleData; i++) {
        SampleData[i] = SoundLoadSample(sample_pointers[i]);
    }
}

SAMPLE_DATA *SoundLoadSample(char *content)
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
    if (SoundMakeSample(sample_data)) {
        return sample_data;
    }
    return NULL;
}

int32_t SoundMakeSample(SAMPLE_DATA *sample_data)
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

    return 1;
}

// original name: S_CDVolume
void S_MusicVolume(int16_t volume)
{
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    auxSetVolume(AuxDeviceID, volume_aux);
}

// original name: CDPlay
int32_t MusicPlay(int16_t track)
{
    if (track < 2) {
        return 0;
    }

    if (track >= 57) {
        CDTrackLooped = track;
    }

    CDLoop = 0;

    uint32_t volume = OptionMusicVolume * 0xFFFF / 10;
    volume |= volume << 16;
    auxSetVolume(AuxDeviceID, volume);

    MCI_SET_PARMS set_parms;
    set_parms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (mciSendCommandA(
            MCIDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&set_parms)) {
        return 0;
    }

    MCI_PLAY_PARMS open_parms;
    open_parms.dwFrom = track;
    open_parms.dwCallback = (DWORD_PTR)TombHWND;

    DWORD_PTR dwFlags = MCI_NOTIFY | MCI_FROM;
    if (track != CDNumTracks) {
        open_parms.dwTo = track + 1;
        dwFlags |= MCI_TO;
    }

    if (mciSendCommandA(
            MCIDeviceID, MCI_PLAY, dwFlags, (DWORD_PTR)&open_parms)) {
        return 0;
    }

    return 1;
}

// original name: CDPlayLooped
int32_t MusicPlayLooped()
{
    if (CDLoop && CDTrackLooped > 0) {
        MusicPlay(CDTrackLooped);
        return 0;
    }

    return CDLoop;
}

// original name: S_CDPlay
int32_t S_MusicPlay(int16_t track_id)
{
    if (T1MConfig.fix_secrets_killing_music && track_id == 13) {
        SoundEffect(SFX_SECRET, NULL, SPM_ALWAYS);
        return 1;
    }

    if (track_id == 0) {
        S_MusicStop();
        return 0;
    }

    if (track_id == 5) {
        return 0;
    }

    CDTrack = track_id;
    return MusicPlay(track_id);
}

// original name: S_CDStop
int32_t S_MusicStop()
{
    CDTrack = 0;
    CDTrackLooped = 0;
    CDLoop = 0;

    MCI_GENERIC_PARMS gen_parms;
    return !mciSendCommandA(
        MCIDeviceID, MCI_STOP, MCI_WAIT, (DWORD_PTR)&gen_parms);
}

// original name: S_CDLoop
void S_MusicLoop()
{
    CDLoop = 1;
}

void *S_SoundPlaySample(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan)
{
    if (!SoundIsActive) {
        return 0;
    }
    return SoundPlaySample(
        sample_id, (MnSoundMasterVolume * volume) >> 6, pitch, pan, 0);
}

void *S_SoundPlaySampleLooped(
    int32_t sample_id, uint16_t volume, uint16_t pitch, int16_t pan)
{
    if (!SoundIsActive) {
        return 0;
    }
    return SoundPlaySample(
        sample_id, (MnSoundMasterVolume * volume) >> 6, pitch, pan, 1);
}

void S_SoundStopAllSamples()
{
    if (!SoundIsActive) {
        return;
    }

    for (int i = 0; i < NumSampleData; i++) {
        SAMPLE_DATA *sample = SampleData[i];
        if (sample) {
            S_SoundStopSample(sample->handle);
        }
    }
}

void S_SoundStopSample(void *handle)
{
    if (!SoundIsActive) {
        return;
    }
    if (handle == SOUND_INVALID_HANDLE) {
        return;
    }
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    HRESULT result = IDirectSoundBuffer_Stop(buffer);
    if (result != DS_OK) {
        LOG_ERROR("Error while calling IDirectSoundBuffer_Stop: 0x%lx", result);
    }
}

void S_SoundSetPanAndVolume(void *handle, int16_t pan, int16_t volume)
{
    if (!SoundIsActive) {
        return;
    }
    if (handle == SOUND_INVALID_HANDLE) {
        return;
    }
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    HRESULT result;
    result = IDirectSoundBuffer_SetVolume(
        buffer, ConvertVolumeToDecibel((MnSoundMasterVolume * volume) >> 6));
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

int32_t S_SoundSampleIsPlaying(void *handle)
{
    if (!SoundIsActive) {
        return 0;
    }
    if (handle == SOUND_INVALID_HANDLE) {
        return 0;
    }
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    DWORD status;
    HRESULT result = IDirectSoundBuffer_GetStatus(buffer, &status);
    if (result != DS_OK) {
        LOG_ERROR(
            "Error while calling IDirectSoundBuffer_GetStatus: 0x%lx", result);
        return 0;
    }
    return status == DSBSTATUS_PLAYING;
}

void T1MInjectSpecificSndPC()
{
    INJECT(0x00419E90, SoundInit);
    INJECT(0x00419F50, SoundMakeSample);
    INJECT(0x00437C00, SoundLoadSamples);
    INJECT(0x00437CB0, SoundLoadSample);
    INJECT(0x00437FB0, MusicPlay);
    INJECT(0x004380B0, S_MusicLoop);
    INJECT(0x004380C0, MusicPlayLooped);
    INJECT(0x00438BF0, S_SoundPlaySample);
    INJECT(0x00438C40, S_SoundPlaySampleLooped);
    INJECT(0x00438CA0, S_SoundSampleIsPlaying);
    INJECT(0x00438CC0, S_SoundStopAllSamples);
    INJECT(0x00438CD0, S_SoundStopSample);
    INJECT(0x00438CF0, S_SoundSetPanAndVolume);
    INJECT(0x00438D40, S_MusicPlay);
    INJECT(0x00438E40, S_MusicStop);
    INJECT(0x00439030, S_MusicPlay);

    // NOTE: this is a nullsub in OG and is called in many different places
    // for many different purposes so it's not injected.
    // INJECT(0x00437F30, S_MusicVolume);
}
