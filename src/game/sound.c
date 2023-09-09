#include "game/sound.h"

#include "config.h"
#include "game/random.h"
#include "game/room.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "specific/s_audio.h"
#include "util.h"

#include <math.h>
#include <stddef.h>

#define MAX_PLAYING_FX AUDIO_MAX_ACTIVE_SAMPLES
#define MAX_AMBIENT_FX 8
#define DECIBEL_LUT_SIZE 512
#define SOUND_FLIPFLAG 0x40
#define SOUND_UNFLIPFLAG 0x80
#define SOUND_RANGE 8
#define SOUND_RANGE_MULT_CONSTANT 4
#define SOUND_RADIUS (SOUND_RANGE * WALL_L)
#define SOUND_MAX_VOLUME ((SOUND_RADIUS * SOUND_RANGE_MULT_CONSTANT) - 1)
#define SOUND_MAX_VOLUME_CHANGE 0x2000
#define SOUND_MAX_PITCH_CHANGE 10
#define SOUND_NOT_AUDIBLE -1

typedef struct SOUND_SLOT {
    int sound_id;
    PHD_3DPOS *pos;
    uint32_t loudness;
    int16_t volume;
    int16_t pan;
    int16_t fxnum;
    int16_t flags;
} SOUND_SLOT;

typedef enum SOUND_MODE {
    SOUND_MODE_WAIT = 0,
    SOUND_MODE_RESTART = 1,
    SOUND_MODE_AMBIENT = 2,
} SOUND_MODE;

typedef enum SOUND_FLAG {
    SOUND_FLAG_UNUSED = 0,
    SOUND_FLAG_USED = 1 << 0,
    SOUND_FLAG_AMBIENT = 1 << 1,
    SOUND_FLAG_RESTARTED = 1 << 2,
} SOUND_FLAG;

static SOUND_SLOT m_SFXPlaying[MAX_PLAYING_FX] = { 0 };
static int32_t m_MasterVolume = 0;
static int32_t m_MasterVolumeDefault = 0;
static int16_t m_AmbientLookup[MAX_AMBIENT_FX] = { 0 };
static int32_t m_AmbientLookupIdx = 0;
static int m_DecibelLUT[DECIBEL_LUT_SIZE] = { 0 };
static bool m_SoundIsActive = false;

static SOUND_SLOT *Sound_GetSlot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode);
static void Sound_UpdateSlotParams(SOUND_SLOT *slot);
static void Sound_ClearSlot(SOUND_SLOT *slot);
static void Sound_ClearSlotHandles(SOUND_SLOT *slot);
static void Sound_ResetAmbientLoudness(void);

static int32_t Sound_ConvertVolumeToDecibel(int volume)
{
    return m_DecibelLUT
        [(volume & SOUND_MAX_VOLUME) * DECIBEL_LUT_SIZE
         / (SOUND_MAX_VOLUME + 1)];
}

static int32_t Sound_ConvertPanToDecibel(uint16_t pan)
{
    int32_t result = sin((pan / 32767.0) * M_PI) * (DECIBEL_LUT_SIZE / 2);
    if (result > 0) {
        return -m_DecibelLUT[DECIBEL_LUT_SIZE - result];
    } else if (result < 0) {
        return m_DecibelLUT[DECIBEL_LUT_SIZE + result];
    } else {
        return 0;
    }
}

static float Sound_CalcPitch(int pitch)
{
    return pitch / 100.0f;
}

static SOUND_SLOT *Sound_GetSlot(
    int32_t sfx_num, uint32_t loudness, PHD_3DPOS *pos, int16_t mode)
{
    switch (mode) {
    case SOUND_MODE_WAIT:
    case SOUND_MODE_RESTART: {
        SOUND_SLOT *last_free_slot = NULL;
        for (int i = m_AmbientLookupIdx; i < MAX_PLAYING_FX; i++) {
            SOUND_SLOT *result = &m_SFXPlaying[i];
            if ((result->flags & SOUND_FLAG_USED) && result->fxnum == sfx_num
                && result->pos == pos) {
                result->flags |= SOUND_FLAG_RESTARTED;
                return result;
            } else if (result->flags == SOUND_FLAG_UNUSED) {
                last_free_slot = result;
            }
        }
        return last_free_slot;
    }

    case SOUND_MODE_AMBIENT:
        for (int i = 0; i < MAX_AMBIENT_FX; i++) {
            if (m_AmbientLookup[i] == sfx_num) {
                SOUND_SLOT *result = &m_SFXPlaying[i];
                if (result->flags != SOUND_FLAG_UNUSED
                    && result->loudness <= loudness) {
                    return NULL;
                }
                return result;
            }
        }
        break;
    }

    return NULL;
}

static void Sound_UpdateSlotParams(SOUND_SLOT *slot)
{
    SAMPLE_INFO *s = &g_SampleInfos[g_SampleLUT[slot->fxnum]];

    int32_t x = slot->pos->x - g_Camera.target.x;
    int32_t y = slot->pos->y - g_Camera.target.y;
    int32_t z = slot->pos->z - g_Camera.target.z;
    if (ABS(x) > SOUND_RADIUS || ABS(y) > SOUND_RADIUS
        || ABS(z) > SOUND_RADIUS) {
        slot->volume = 0;
        return;
    }

    uint32_t distance = SQUARE(x) + SQUARE(y) + SQUARE(z);
    int32_t volume =
        s->volume - Math_Sqrt(distance) * SOUND_RANGE_MULT_CONSTANT;
    if (volume < 0) {
        slot->volume = 0;
        return;
    }

    CLAMPG(volume, SOUND_MAX_VOLUME);

    slot->volume = volume;

    if (!distance || (s->flags & SAMPLE_FLAG_NO_PAN)) {
        slot->pan = 0;
        return;
    }

    int16_t angle = Math_Atan(
        slot->pos->z - g_LaraItem->pos.z, slot->pos->x - g_LaraItem->pos.x);
    angle -= g_LaraItem->pos.y_rot + g_Lara.torso_y_rot + g_Lara.head_y_rot;
    slot->pan = angle;
}

static void Sound_ClearSlot(SOUND_SLOT *slot)
{
    slot->sound_id = AUDIO_NO_SOUND;
    slot->pos = NULL;
    slot->flags = SOUND_FLAG_UNUSED;
    slot->volume = 0;
    slot->pan = 0;
    slot->loudness = SOUND_NOT_AUDIBLE;
    slot->fxnum = -1;
}

static void Sound_ClearSlotHandles(SOUND_SLOT *slot)
{
    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *rslot = &m_SFXPlaying[i];
        if (rslot != slot && rslot->sound_id == slot->sound_id) {
            rslot->sound_id = AUDIO_NO_SOUND;
        }
    }
}

static void Sound_ResetAmbientLoudness(void)
{
    if (!m_SoundIsActive) {
        return;
    }

    for (int i = 0; i < m_AmbientLookupIdx; i++) {
        SOUND_SLOT *slot = &m_SFXPlaying[i];
        slot->loudness = SOUND_NOT_AUDIBLE;
    }
}

bool Sound_Init(void)
{
    m_DecibelLUT[0] = -10000;
    for (int i = 1; i < DECIBEL_LUT_SIZE; i++) {
        m_DecibelLUT[i] = (log2(1.0 / DECIBEL_LUT_SIZE) - log2(1.0 / i)) * 1000;
    }

    m_MasterVolume = 32;
    m_MasterVolumeDefault = 32;
    m_SoundIsActive = S_Audio_Init();
    return m_SoundIsActive;
}

void Sound_Shutdown(void)
{
    S_Audio_Shutdown();
}

void Sound_UpdateEffects(void)
{
    if (!m_SoundIsActive) {
        return;
    }

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *slot = &m_SFXPlaying[i];
        if (!(slot->flags & SOUND_FLAG_USED)) {
            continue;
        }

        if (slot->flags & SOUND_FLAG_AMBIENT) {
            if (slot->loudness != (uint32_t)SOUND_NOT_AUDIBLE
                && slot->sound_id != AUDIO_NO_SOUND) {
                S_Audio_SampleSoundSetPan(
                    slot->sound_id, Sound_ConvertPanToDecibel(slot->pan));
                S_Audio_SampleSoundSetVolume(
                    slot->sound_id,
                    Sound_ConvertVolumeToDecibel(
                        (m_MasterVolume * slot->volume) >> 6));
            } else {
                if (slot->sound_id != AUDIO_NO_SOUND) {
                    S_Audio_SampleSoundClose(slot->sound_id);
                }
                Sound_ClearSlot(slot);
            }
        } else if (S_Audio_SampleSoundIsPlaying(slot->sound_id)) {
            if (slot->pos != NULL) {
                Sound_UpdateSlotParams(slot);
                if (slot->volume > 0 && slot->sound_id != AUDIO_NO_SOUND) {
                    S_Audio_SampleSoundSetPan(
                        slot->sound_id, Sound_ConvertPanToDecibel(slot->pan));
                    S_Audio_SampleSoundSetVolume(
                        slot->sound_id,
                        Sound_ConvertVolumeToDecibel(
                            (m_MasterVolume * slot->volume) >> 6));
                } else {
                    if (slot->sound_id != AUDIO_NO_SOUND) {
                        S_Audio_SampleSoundClose(slot->sound_id);
                    }
                    Sound_ClearSlot(slot);
                }
            }
        } else {
            Sound_ClearSlot(slot);
        }
    }
}

bool Sound_Effect(int32_t sfx_num, PHD_3DPOS *pos, uint32_t flags)
{
    if (!m_SoundIsActive) {
        return false;
    }

    if (flags != SPM_ALWAYS
        && (flags & SPM_UNDERWATER)
            != (g_RoomInfo[g_Camera.pos.room_number].flags & RF_UNDERWATER)) {
        return false;
    }

    if (g_SampleLUT[sfx_num] < 0) {
        return false;
    }

    SAMPLE_INFO *s = &g_SampleInfos[g_SampleLUT[sfx_num]];
    if (s->randomness && Random_GetDraw() > (int32_t)s->randomness) {
        return false;
    }

    int32_t pan = 0x7FFF;
    int32_t mode = s->flags & 3;
    uint32_t distance;
    if (pos) {
        int32_t x = pos->x - g_Camera.target.x;
        int32_t y = pos->y - g_Camera.target.y;
        int32_t z = pos->z - g_Camera.target.z;
        if (ABS(x) > SOUND_RADIUS || ABS(y) > SOUND_RADIUS
            || ABS(z) > SOUND_RADIUS) {
            return false;
        }
        distance = SQUARE(x) + SQUARE(y) + SQUARE(z);
        if (!distance) {
            pan = 0;
        }
    } else {
        distance = 0;
        pan = 0;
    }
    distance = Math_Sqrt(distance);

    int32_t volume = s->volume - distance * SOUND_RANGE_MULT_CONSTANT;
    if (s->flags & SAMPLE_FLAG_VOLUME_WIBBLE) {
        volume -= Random_GetDraw() * SOUND_MAX_VOLUME_CHANGE >> 15;
    }

    if (s->flags & SAMPLE_FLAG_NO_PAN) {
        pan = 0;
    }

    if (volume <= 0 && mode != SOUND_MODE_AMBIENT) {
        return false;
    }

    if (pan) {
        int16_t angle =
            Math_Atan(pos->z - g_LaraItem->pos.z, pos->x - g_LaraItem->pos.x);
        angle -= g_LaraItem->pos.y_rot + g_Lara.torso_y_rot + g_Lara.head_y_rot;
        pan = angle;
    }

    int32_t pitch = 100;
    if (g_Config.enable_pitched_sounds
        && (s->flags & SAMPLE_FLAG_PITCH_WIBBLE)) {
        pitch += ((Random_GetDraw() * SOUND_MAX_PITCH_CHANGE) / 0x4000)
            - SOUND_MAX_PITCH_CHANGE;
    }

    int32_t vars = (s->flags >> 2) & 15;
    int32_t sfx_id = s->number;
    if (vars != 1) {
        sfx_id += (Random_GetDraw() * vars) / 0x8000;
    }

    CLAMPG(volume, SOUND_MAX_VOLUME);

    volume = (m_MasterVolume * volume) >> 6;

    switch (mode) {
    case SOUND_MODE_WAIT: {
        SOUND_SLOT *fxslot = Sound_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->flags & SOUND_FLAG_RESTARTED) {
            fxslot->flags &= ~SOUND_FLAG_RESTARTED;
            return true;
        }
        fxslot->sound_id = S_Audio_SampleSoundPlay(
            sfx_id, Sound_ConvertVolumeToDecibel(volume),
            Sound_CalcPitch(pitch), Sound_ConvertPanToDecibel(pan), false);
        if (fxslot->sound_id == AUDIO_NO_SOUND) {
            return false;
        }
        Sound_ClearSlotHandles(fxslot);
        fxslot->flags = SOUND_FLAG_USED;
        fxslot->fxnum = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_MODE_RESTART: {
        SOUND_SLOT *fxslot = Sound_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->flags & SOUND_FLAG_RESTARTED) {
            S_Audio_SampleSoundClose(fxslot->sound_id);
            fxslot->sound_id = S_Audio_SampleSoundPlay(
                sfx_id, Sound_ConvertVolumeToDecibel(volume),
                Sound_CalcPitch(pitch), Sound_ConvertPanToDecibel(pan), false);

            Sound_ClearSlotHandles(fxslot);
            return true;
        }
        fxslot->sound_id = S_Audio_SampleSoundPlay(
            sfx_id, Sound_ConvertVolumeToDecibel(volume),
            Sound_CalcPitch(pitch), Sound_ConvertPanToDecibel(pan), false);
        if (fxslot->sound_id == AUDIO_NO_SOUND) {
            return false;
        }
        Sound_ClearSlotHandles(fxslot);
        fxslot->flags = SOUND_FLAG_USED;
        fxslot->fxnum = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_MODE_AMBIENT: {
        uint32_t loudness = distance;
        SOUND_SLOT *fxslot = Sound_GetSlot(sfx_num, loudness, pos, mode);
        if (!fxslot) {
            return false;
        }
        fxslot->pos = pos;
        if (fxslot->flags & SOUND_FLAG_AMBIENT) {
            if (volume > 0) {
                fxslot->loudness = loudness;
                fxslot->pan = pan;
                fxslot->volume = volume;
            } else {
                fxslot->loudness = SOUND_NOT_AUDIBLE;
                fxslot->volume = 0;
            }
            return true;
        }

        if (volume > 0) {
            fxslot->sound_id = S_Audio_SampleSoundPlay(
                sfx_id, Sound_ConvertVolumeToDecibel(volume),
                Sound_CalcPitch(pitch), Sound_ConvertPanToDecibel(pan), true);
            if (fxslot->sound_id == AUDIO_NO_SOUND) {
                Sound_ClearSlot(fxslot);
                return false;
            }
            Sound_ClearSlotHandles(fxslot);
            fxslot->loudness = loudness;
            fxslot->fxnum = sfx_num;
            fxslot->pan = pan;
            fxslot->volume = volume;
            fxslot->flags |= SOUND_FLAG_AMBIENT | SOUND_FLAG_USED;
            fxslot->pos = pos;
            return true;
        }

        fxslot->loudness = SOUND_NOT_AUDIBLE;
        return true;
    }
    }

    return false;
}

bool Sound_StopEffect(int32_t sfx_num, PHD_3DPOS *pos)
{
    if (!m_SoundIsActive) {
        return false;
    }

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *slot = &m_SFXPlaying[i];
        if ((slot->flags & SOUND_FLAG_USED)
            && S_Audio_SampleSoundIsPlaying(slot->sound_id)) {
            if ((!pos && slot->fxnum == sfx_num)
                || (pos && sfx_num >= 0 && slot->fxnum == sfx_num)
                || (pos && sfx_num < 0)) {
                S_Audio_SampleSoundClose(slot->sound_id);
                Sound_ClearSlot(slot);
                return true;
            }
        }
    }

    return false;
}

void Sound_ResetEffects(void)
{
    if (!m_SoundIsActive) {
        return;
    }
    m_MasterVolume = m_MasterVolumeDefault;

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        Sound_ClearSlot(&m_SFXPlaying[i]);
    }

    Sound_StopAllSamples();

    m_AmbientLookupIdx = 0;

    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (g_SampleLUT[i] < 0) {
            continue;
        }
        SAMPLE_INFO *s = &g_SampleInfos[g_SampleLUT[i]];
        if (s->volume < 0) {
            Shell_ExitSystemFmt(
                "sample info for effect %d has incorrect volume(%d)", i,
                s->volume);
        }

        int32_t mode = s->flags & 3;
        if (mode == SOUND_MODE_AMBIENT) {
            if (m_AmbientLookupIdx >= MAX_AMBIENT_FX) {
                Shell_ExitSystem("Ran out of ambient fx slots in "
                                 "Sound_ResetEffects()");
            }
            m_AmbientLookup[m_AmbientLookupIdx] = i;
            m_AmbientLookupIdx++;
        }
    }
}

void Sound_StopAmbientSounds(void)
{
    if (!m_SoundIsActive) {
        return;
    }

    for (int i = 0; i < m_AmbientLookupIdx; i++) {
        SOUND_SLOT *slot = &m_SFXPlaying[i];
        if (S_Audio_SampleSoundIsPlaying(slot->sound_id)) {
            S_Audio_SampleSoundClose(slot->sound_id);
            Sound_ClearSlot(slot);
        }
    }
}

void Sound_LoadSamples(
    size_t num_samples, const char **sample_pointers, size_t *sizes)
{
    S_Audio_SamplesLoad(num_samples, sample_pointers, sizes);
}

void Sound_PauseAll(void)
{
    S_Audio_SampleSoundPauseAll();
}

void Sound_UnpauseAll(void)
{
    S_Audio_SampleSoundUnpauseAll();
}

void Sound_StopAllSamples(void)
{
    S_Audio_SampleSoundCloseAll();
}

void Sound_SetMasterVolume(int8_t volume)
{
    int8_t raw_volume = volume ? 6 * volume + 3 : 0;
    m_MasterVolumeDefault = raw_volume & 0x3F;
    m_MasterVolume = raw_volume & 0x3F;
}

void Sound_ResetAmbient(void)
{
    Sound_ResetAmbientLoudness();

    for (int i = 0; i < g_NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &g_SoundEffectsTable[i];
        if (g_FlipStatus && (sound->flags & SOUND_FLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        } else if (!g_FlipStatus && (sound->flags & SOUND_UNFLIPFLAG)) {
            Sound_Effect(sound->data, (PHD_3DPOS *)sound, SPM_NORMAL);
        }
    }
}
