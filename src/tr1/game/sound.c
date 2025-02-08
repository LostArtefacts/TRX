#include "game/sound.h"

#include "game/camera.h"
#include "game/random.h"
#include "game/room.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/engine/audio.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#include <math.h>

#define MAX_PLAYING_FX AUDIO_MAX_ACTIVE_SAMPLES
#define MAX_AMBIENT_FX 8
#define DECIBEL_LUT_SIZE 512
#define SOUND_RANGE 8
#define SOUND_RANGE_MULT_CONSTANT 4
#define SOUND_RADIUS (SOUND_RANGE * WALL_L)
#define SOUND_MAX_VOLUME ((SOUND_RADIUS * SOUND_RANGE_MULT_CONSTANT) - 1)
#define SOUND_MAX_VOLUME_CHANGE 0x2000
#define SOUND_MAX_PITCH_CHANGE 10
#define SOUND_NOT_AUDIBLE -1

typedef struct {
    int sound_id;
    const XYZ_32 *pos;
    uint32_t loudness;
    int16_t volume;
    int16_t pan;
    SOUND_EFFECT_ID effect_num;
    int16_t flags;
} SOUND_SLOT;

typedef enum {
    SOUND_MODE_WAIT = 0,
    SOUND_MODE_RESTART = 1,
    SOUND_MODE_AMBIENT = 2,
} SOUND_MODE;

typedef enum {
    SOUND_FLAG_UNUSED = 0,
    SOUND_FLAG_USED = 1 << 0,
    SOUND_FLAG_AMBIENT = 1 << 1,
    SOUND_FLAG_RESTARTED = 1 << 2,
} SOUND_FLAG;

static SOUND_SLOT m_SFXPlaying[MAX_PLAYING_FX] = {};
static int32_t m_MasterVolume = 0;
static int32_t m_MasterVolumeDefault = 0;
static int16_t m_AmbientLookup[MAX_AMBIENT_FX] = {};
static int32_t m_AmbientLookupIdx = 0;
static int m_DecibelLUT[DECIBEL_LUT_SIZE] = {};
static bool m_SoundIsActive = false;

static int32_t M_ConvertVolumeToDecibel(int volume);
static int32_t M_ConvertPanToDecibel(uint16_t pan);
static float M_CalcPitch(int pitch);

static SOUND_SLOT *M_GetSlot(
    int32_t sfx_num, uint32_t loudness, const XYZ_32 *pos, int16_t mode);
static void M_UpdateSlotParams(SOUND_SLOT *slot);
static void M_ClearSlot(SOUND_SLOT *slot);
static void M_ClearSlotHandles(SOUND_SLOT *slot);
static void M_ResetAmbientLoudness(void);

static int32_t M_ConvertVolumeToDecibel(int volume)
{
    return m_DecibelLUT
        [(volume & SOUND_MAX_VOLUME) * DECIBEL_LUT_SIZE
         / (SOUND_MAX_VOLUME + 1)];
}

static int32_t M_ConvertPanToDecibel(uint16_t pan)
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

static float M_CalcPitch(int pitch)
{
    return pitch / 100.0f;
}

static SOUND_SLOT *M_GetSlot(
    int32_t sfx_num, uint32_t loudness, const XYZ_32 *pos, int16_t mode)
{
    switch (mode) {
    case SOUND_MODE_WAIT:
    case SOUND_MODE_RESTART: {
        SOUND_SLOT *last_free_slot = nullptr;
        for (int i = m_AmbientLookupIdx; i < MAX_PLAYING_FX; i++) {
            SOUND_SLOT *result = &m_SFXPlaying[i];
            if ((result->flags & SOUND_FLAG_USED)
                && result->effect_num == sfx_num && result->pos == pos) {
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
                    return nullptr;
                }
                return result;
            }
        }
        break;
    }

    return nullptr;
}

static void M_UpdateSlotParams(SOUND_SLOT *slot)
{
    const SAMPLE_INFO *const info = Sound_GetSampleInfo(slot->effect_num);

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
        info->volume - Math_Sqrt(distance) * SOUND_RANGE_MULT_CONSTANT;
    if (volume < 0) {
        slot->volume = 0;
        return;
    }

    CLAMPG(volume, SOUND_MAX_VOLUME);

    slot->volume = volume;

    if (!distance || (info->flags & SAMPLE_FLAG_NO_PAN)) {
        slot->pan = 0;
        return;
    }

    int16_t angle = Math_Atan(
        slot->pos->z - g_LaraItem->pos.z, slot->pos->x - g_LaraItem->pos.x);
    angle -= g_LaraItem->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    slot->pan = angle;
}

static void M_ClearSlot(SOUND_SLOT *slot)
{
    slot->sound_id = AUDIO_NO_SOUND;
    slot->pos = nullptr;
    slot->flags = SOUND_FLAG_UNUSED;
    slot->volume = 0;
    slot->pan = 0;
    slot->loudness = SOUND_NOT_AUDIBLE;
    slot->effect_num = SFX_INVALID;
}

static void M_ClearSlotHandles(SOUND_SLOT *slot)
{
    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *rslot = &m_SFXPlaying[i];
        if (rslot != slot && rslot->sound_id == slot->sound_id) {
            rslot->sound_id = AUDIO_NO_SOUND;
        }
    }
}

static void M_ResetAmbientLoudness(void)
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
    m_SoundIsActive = Audio_Init();
    return m_SoundIsActive;
}

void Sound_Shutdown(void)
{
    Audio_Shutdown();
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
                Audio_Sample_SetPan(
                    slot->sound_id, M_ConvertPanToDecibel(slot->pan));
                Audio_Sample_SetVolume(
                    slot->sound_id,
                    M_ConvertVolumeToDecibel(
                        (m_MasterVolume * slot->volume) >> 6));
            } else {
                if (slot->sound_id != AUDIO_NO_SOUND) {
                    Audio_Sample_Close(slot->sound_id);
                }
                M_ClearSlot(slot);
            }
        } else if (Audio_Sample_IsPlaying(slot->sound_id)) {
            if (slot->pos != nullptr) {
                M_UpdateSlotParams(slot);
                if (slot->volume > 0 && slot->sound_id != AUDIO_NO_SOUND) {
                    Audio_Sample_SetPan(
                        slot->sound_id, M_ConvertPanToDecibel(slot->pan));
                    Audio_Sample_SetVolume(
                        slot->sound_id,
                        M_ConvertVolumeToDecibel(
                            (m_MasterVolume * slot->volume) >> 6));
                } else {
                    if (slot->sound_id != AUDIO_NO_SOUND) {
                        Audio_Sample_Close(slot->sound_id);
                    }
                    M_ClearSlot(slot);
                }
            }
        } else {
            M_ClearSlot(slot);
        }
    }
}

bool Sound_Effect(
    const SOUND_EFFECT_ID sfx_num, const XYZ_32 *const pos,
    const uint32_t flags)
{
    if (!m_SoundIsActive) {
        return false;
    }

    if (flags != SPM_ALWAYS
        && (flags & SPM_UNDERWATER)
            != (Room_Get(g_Camera.pos.room_num)->flags & RF_UNDERWATER)) {
        return false;
    }

    const SAMPLE_INFO *const info = Sound_GetSampleInfo(sfx_num);
    if (info == nullptr) {
        return false;
    }

    if (info->randomness && Random_GetDraw() > (int32_t)info->randomness) {
        return false;
    }

    int32_t pan = 0x7FFF;
    int32_t mode = info->flags & 3;
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

    int32_t volume = info->volume - distance * SOUND_RANGE_MULT_CONSTANT;
    if (info->flags & SAMPLE_FLAG_VOLUME_WIBBLE) {
        volume -= Random_GetDraw() * SOUND_MAX_VOLUME_CHANGE >> 15;
    }

    if (info->flags & SAMPLE_FLAG_NO_PAN) {
        pan = 0;
    }

    if (volume <= 0 && mode != SOUND_MODE_AMBIENT) {
        return false;
    }

    if (pan) {
        int16_t angle =
            Math_Atan(pos->z - g_LaraItem->pos.z, pos->x - g_LaraItem->pos.x);
        angle -= g_LaraItem->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
        pan = angle;
    }

    int32_t pitch = 100;
    if (g_Config.audio.enable_pitched_sounds
        && (info->flags & SAMPLE_FLAG_PITCH_WIBBLE)) {
        pitch += ((Random_GetDraw() * SOUND_MAX_PITCH_CHANGE) / 0x4000)
            - SOUND_MAX_PITCH_CHANGE;
    }

    int32_t vars = (info->flags >> 2) & 15;
    int32_t sfx_id = info->number;
    if (vars != 1) {
        sfx_id += (Random_GetDraw() * vars) / 0x8000;
    }

    CLAMPG(volume, SOUND_MAX_VOLUME);

    volume = (m_MasterVolume * volume) >> 6;

    switch (mode) {
    case SOUND_MODE_WAIT: {
        SOUND_SLOT *fxslot = M_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->flags & SOUND_FLAG_RESTARTED) {
            fxslot->flags &= ~SOUND_FLAG_RESTARTED;
            return true;
        }
        fxslot->sound_id = Audio_Sample_Play(
            sfx_id, M_ConvertVolumeToDecibel(volume), M_CalcPitch(pitch),
            M_ConvertPanToDecibel(pan), false);
        if (fxslot->sound_id == AUDIO_NO_SOUND) {
            return false;
        }
        M_ClearSlotHandles(fxslot);
        fxslot->flags = SOUND_FLAG_USED;
        fxslot->effect_num = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_MODE_RESTART: {
        SOUND_SLOT *fxslot = M_GetSlot(sfx_num, 0, pos, mode);
        if (!fxslot) {
            return false;
        }
        if (fxslot->flags & SOUND_FLAG_RESTARTED) {
            Audio_Sample_Close(fxslot->sound_id);
            fxslot->sound_id = Audio_Sample_Play(
                sfx_id, M_ConvertVolumeToDecibel(volume), M_CalcPitch(pitch),
                M_ConvertPanToDecibel(pan), false);

            M_ClearSlotHandles(fxslot);
            return true;
        }
        fxslot->sound_id = Audio_Sample_Play(
            sfx_id, M_ConvertVolumeToDecibel(volume), M_CalcPitch(pitch),
            M_ConvertPanToDecibel(pan), false);
        if (fxslot->sound_id == AUDIO_NO_SOUND) {
            return false;
        }
        M_ClearSlotHandles(fxslot);
        fxslot->flags = SOUND_FLAG_USED;
        fxslot->effect_num = sfx_num;
        fxslot->pos = pos;
        return true;
    }

    case SOUND_MODE_AMBIENT: {
        uint32_t loudness = distance;
        SOUND_SLOT *fxslot = M_GetSlot(sfx_num, loudness, pos, mode);
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
            fxslot->sound_id = Audio_Sample_Play(
                sfx_id, M_ConvertVolumeToDecibel(volume), M_CalcPitch(pitch),
                M_ConvertPanToDecibel(pan), true);
            if (fxslot->sound_id == AUDIO_NO_SOUND) {
                M_ClearSlot(fxslot);
                return false;
            }
            M_ClearSlotHandles(fxslot);
            fxslot->loudness = loudness;
            fxslot->effect_num = sfx_num;
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

bool Sound_StopEffect(const SOUND_EFFECT_ID sfx_num, const XYZ_32 *const pos)
{
    if (!m_SoundIsActive) {
        return false;
    }

    for (int i = 0; i < MAX_PLAYING_FX; i++) {
        SOUND_SLOT *slot = &m_SFXPlaying[i];
        if ((slot->flags & SOUND_FLAG_USED)
            && Audio_Sample_IsPlaying(slot->sound_id)) {
            if ((!pos && slot->effect_num == sfx_num)
                || (pos && sfx_num >= 0 && slot->effect_num == sfx_num)
                || (pos && sfx_num < 0)) {
                Audio_Sample_Close(slot->sound_id);
                M_ClearSlot(slot);
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
        M_ClearSlot(&m_SFXPlaying[i]);
    }

    Sound_StopAll();

    m_AmbientLookupIdx = 0;

    for (int32_t i = 0; i < SFX_NUMBER_OF; i++) {
        const SAMPLE_INFO *const info = Sound_GetSampleInfo(i);
        if (info == nullptr) {
            continue;
        }
        if (info->volume < 0) {
            Shell_ExitSystemFmt(
                "sample info for effect %d has incorrect volume(%d)", i,
                info->volume);
        }

        const int32_t mode = info->flags & 3;
        if (mode == SOUND_MODE_AMBIENT) {
            if (m_AmbientLookupIdx >= MAX_AMBIENT_FX) {
                Shell_ExitSystem(
                    "Ran out of ambient effect slots in Sound_ResetEffects()");
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
        if (Audio_Sample_IsPlaying(slot->sound_id)) {
            Audio_Sample_Close(slot->sound_id);
            M_ClearSlot(slot);
        }
    }
}

void Sound_LoadSamples(
    size_t num_samples, const char **sample_pointers, size_t *sizes)
{
    Audio_Sample_LoadMany(num_samples, sample_pointers, sizes);
}

void Sound_StopAll(void)
{
    Audio_Sample_CloseAll();
}

void Sound_SetMasterVolume(int8_t volume)
{
    int8_t raw_volume = volume ? 6 * volume + 3 : 0;
    m_MasterVolumeDefault = raw_volume & 0x3F;
    m_MasterVolume = raw_volume & 0x3F;
}

int8_t Sound_GetMasterVolume(void)
{
    return (m_MasterVolume - 3) / 6;
}

int32_t Sound_GetMinVolume(void)
{
    return 0;
}

int32_t Sound_GetMaxVolume(void)
{
    return 10;
}

void Sound_ResetAmbient(void)
{
    M_ResetAmbientLoudness();
    Sound_ResetSources();
}
