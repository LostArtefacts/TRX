#include "specific/sndpc.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>

#define DECIBEL_LUT_SIZE 512

typedef struct DUPE_SOUND_BUFFER {
    SAMPLE_DATA *sample;
    LPDIRECTSOUNDBUFFER buffer;
    struct DUPE_SOUND_BUFFER *next;
} DUPE_SOUND_BUFFER;

static DUPE_SOUND_BUFFER *DupeSoundBufferList = NULL;

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

static LPDIRECTSOUNDBUFFER SoundPlaySample(
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
    IDirectSoundBuffer_GetStatus((LPDIRECTSOUNDBUFFER)sample->handle, &status);
    if (status == DSBSTATUS_PLAYING) {
        buffer = NULL;

        for (DUPE_SOUND_BUFFER *dupe_buffer = DupeSoundBufferList;
             dupe_buffer != NULL; dupe_buffer = dupe_buffer->next) {
            if (dupe_buffer->sample == sample) {
                IDirectSoundBuffer_GetStatus(dupe_buffer->buffer, &status);
                if (status != DSBSTATUS_PLAYING) {
                    buffer = dupe_buffer->buffer;
                    break;
                }
            }
        }

        if (!buffer) {
            LPDIRECTSOUNDBUFFER buffer_new;
            IDirectSound8_DuplicateSoundBuffer(
                DSound, (LPDIRECTSOUNDBUFFER)sample->handle, &buffer_new);

            DUPE_SOUND_BUFFER *dupe_buffer = malloc(sizeof(DUPE_SOUND_BUFFER));
            dupe_buffer->buffer = buffer_new;
            dupe_buffer->next = DupeSoundBufferList;
            DupeSoundBufferList = dupe_buffer;

            buffer = buffer_new;
            TRACE(
                "duplicated sound buffer %p to %p", sample->handle, buffer_new);
        }
    }

    // calculate pitch from sample rate
    int32_t ds_pitch = sample->sample_rate;
    if (pitch != -1) {
        ds_pitch = ds_pitch * pitch / 100;
    }

    IDirectSoundBuffer_SetFrequency(buffer, ds_pitch);
    IDirectSoundBuffer_SetPan(buffer, ConvertPanToDecibel(pan));
    IDirectSoundBuffer_SetVolume(buffer, ConvertVolumeToDecibel(volume));
    IDirectSoundBuffer_SetCurrentPosition(buffer, 0);
    IDirectSoundBuffer_Play(buffer, 0, 0, loop ? DSBPLAY_LOOPING : 0);
    return buffer;
}

int32_t SoundInit()
{
    TRACE("");
    if (DirectSoundCreate(0, &DSound, 0)) {
        return 0;
    }
    if (DSound->lpVtbl->SetCooperativeLevel(DSound, TombHWND, 1)) {
        return 0;
    }
    DecibelLUT[0] = -10000;
    for (int i = 1; i < DECIBEL_LUT_SIZE; i++) {
        DecibelLUT[i] = -9000.0 - log2(1.0 / i) * -1000.0 / log2(0.5);
    }
    return 1;
}

void SoundLoadSamples(char **sample_pointers, int32_t num_samples)
{
    TRACE("");
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
    int8_t encrypted = 0;
    if (content[0] == '~') {
        encrypted = 1;
        content++;
    }

    SAMPLE_DATA *sample_data = malloc(sizeof(SAMPLE_DATA));
    memset(sample_data, 0, sizeof(SAMPLE_DATA));
    sample_data->unk2 = 0;
    if (content[0] == 'R' && content[1] == 'I' && content[2] == 'F'
        && content[3] == 'F') {
        sample_data->data = content + 44;
        sample_data->length = *((int16_t *)content + 20) - 44;
        sample_data->bits_per_sample = *((int16_t *)content + 17);
        sample_data->channels = *((int16_t *)content + 11);
        sample_data->unk1 = 0;
        if (*((int16_t *)content + 17) == 8) {
            sample_data->channels2 = sample_data->channels;
        } else {
            sample_data->channels2 = sample_data->channels * 2;
        }
        sample_data->sample_rate = *((int16_t *)content + 12);
    } else {
        size_t data_size = 0; // TODO: establish if it's needed
        sample_data->data = content;
        sample_data->length = data_size;
        sample_data->channels = 1;
        sample_data->channels2 = 1;
        sample_data->bits_per_sample = 8;
        sample_data->unk1 = 0;
        sample_data->sample_rate = 11025;
        if (encrypted) {
            for (int i = 0; i < data_size; ++i) {
                content[i] ^= 0x80u;
            }
        }
    }

    sample_data->pan = 0;
    sample_data->volume = 0x7FFF;
    if (SoundMakeSample(sample_data)) {
        return sample_data;
    }
    return NULL;
}

void S_CDVolume(int16_t volume)
{
    TRACE("%d", volume);
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    auxSetVolume(AuxDeviceID, volume_aux);
}

int32_t S_CDPlay(int16_t track)
{
    TRACE("%d", track);

    if (T1MConfig.fix_secrets_killing_music && track == 13) {
        SoundEffect(SFX_SECRET, NULL, SPM_ALWAYS);
        return 1;
    }

    if (track == 0) {
        S_CDStop();
        return 0;
    }

    if (track == 5) {
        return 0;
    }

    CDTrack = track;
    return CDPlay(track);
}

int32_t S_StartSyncedAudio(int16_t track)
{
    return S_CDPlay(track);
}

int32_t S_CDStop()
{
    TRACE("");

    CDTrack = 0;
    CDTrackLooped = 0;
    CDLoop = 0;

    MCI_GENERIC_PARMS gen_parms;
    return !mciSendCommandA(
        MCIDeviceID, MCI_STOP, MCI_WAIT, (DWORD_PTR)&gen_parms);
}

int32_t CDPlay(int16_t track)
{
    TRACE("%d", track);

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

void S_CDLoop()
{
    CDLoop = 1;
}

int32_t CDPlayLooped()
{
    TRACE("");

    if (CDLoop && CDTrackLooped > 0) {
        CDPlay(CDTrackLooped);
        return 0;
    }

    return CDLoop;
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
    TRACE("");
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
    TRACE("");
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    if (!SoundIsActive || !SoundInit1 || !SoundInit2) {
        return;
    }
    if (buffer) {
        IDirectSoundBuffer_Stop(buffer);
    }
}

void S_SoundSetPanAndVolume(void *handle, int16_t pan, int16_t volume)
{
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    if (buffer) {
        IDirectSoundBuffer_SetVolume(buffer, ConvertVolumeToDecibel(volume));
        IDirectSoundBuffer_SetPan(buffer, ConvertPanToDecibel(pan));
    }
}

int32_t S_SoundSampleIsPlaying(void *handle)
{
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)handle;
    if (!SoundIsActive || !SoundInit1 || !SoundInit2) {
        return 0;
    }
    if (!buffer) {
        return 0;
    }
    DWORD status;
    IDirectSoundBuffer_GetStatus(buffer, &status);
    return status == DSBSTATUS_PLAYING;
}

void T1MInjectSpecificSndPC()
{
    INJECT(0x00419E90, SoundInit);
    INJECT(0x00437C00, SoundLoadSamples);
    INJECT(0x00437CB0, SoundLoadSample);
    INJECT(0x00437FB0, CDPlay);
    INJECT(0x004380B0, S_CDLoop);
    INJECT(0x004380C0, CDPlayLooped);
    INJECT(0x00438BF0, S_SoundPlaySample);
    INJECT(0x00438C40, S_SoundPlaySampleLooped);
    INJECT(0x00438CA0, S_SoundSampleIsPlaying);
    INJECT(0x00438CC0, S_SoundStopAllSamples);
    INJECT(0x00438CD0, S_SoundStopSample);
    INJECT(0x00438CF0, S_SoundSetPanAndVolume);
    INJECT(0x00438D40, S_CDPlay);
    INJECT(0x00438E40, S_CDStop);
    INJECT(0x00439030, S_StartSyncedAudio);

    // NOTE: this is a nullsub in OG and is called in many different places
    // for many different purposes so it's not injected.
    // INJECT(0x00437F30, S_CDVolume);
}
