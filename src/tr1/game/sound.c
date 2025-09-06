#include "game/sound.h"

#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/engine/audio.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/math.h>
#include <libtrx/game/random.h>
#include <libtrx/utils.h>

#include <math.h>

#define M_MAX_ACTIVE_SOUNDS AUDIO_MAX_ACTIVE_SAMPLES
#define M_MAX_AMBIENT_FX 8
#define M_DECIBEL_LUT_SIZE 512
#define M_SOUND_RANGE 8
#define M_SOUND_RANGE_MULT_CONSTANT 4
#define M_SOUND_RADIUS (M_SOUND_RANGE * WALL_L)
#define M_SOUND_MAX_VOLUME ((M_SOUND_RADIUS * M_SOUND_RANGE_MULT_CONSTANT) - 1)
#define M_SOUND_MAX_VOLUME_CHANGE 0x2000
#define M_SOUND_MAX_PITCH_CHANGE 10
#define M_SOUND_NOT_AUDIBLE -1

typedef struct {
    int32_t handle;
    const XYZ_32 *pos;
    uint32_t loudness;
    int16_t volume;
    int16_t pan;
    SAMPLE_ID sample_id;
    int16_t flags;
} M_ACTIVE_SOUND;

typedef enum {
    SOUND_MODE_WAIT = 0,
    SOUND_MODE_RESTART = 1,
    SOUND_MODE_AMBIENT = 2,
    SOUND_MODE_MASK = 3,
} M_SOUND_MODE;

typedef enum {
    SOUND_FLAG_UNUSED = 0,
    SOUND_FLAG_USED = 1 << 0,
    SOUND_FLAG_AMBIENT = 1 << 1,
    SOUND_FLAG_RESTARTED = 1 << 2,
} M_SOUND_FLAG;

static M_ACTIVE_SOUND m_ActiveSounds[M_MAX_ACTIVE_SOUNDS] = {};
static int16_t m_AmbientLookup[M_MAX_AMBIENT_FX] = {};
static int32_t m_AmbientLookupIdx = 0;

static float M_CalcPitch(int32_t pitch);

static M_ACTIVE_SOUND *M_GetActiveSound(
    SAMPLE_ID sample_id, uint32_t loudness, const XYZ_32 *pos, int16_t mode);
static void M_UpdateActiveSoundParams(M_ACTIVE_SOUND *sound);
static void M_ClearActiveSound(M_ACTIVE_SOUND *sound);
static void M_ClearActiveSoundHandles(const M_ACTIVE_SOUND *sound);
static void M_ResetAmbientLoudness(void);

static float M_CalcPitch(int32_t pitch)
{
    return pitch / 100.0f;
}

static M_ACTIVE_SOUND *M_GetActiveSound(
    const SAMPLE_ID sample_id, const uint32_t loudness, const XYZ_32 *const pos,
    const int16_t mode)
{
    switch (mode) {
    case SOUND_MODE_WAIT:
    case SOUND_MODE_RESTART: {
        M_ACTIVE_SOUND *last_free_sound = nullptr;
        for (int32_t i = m_AmbientLookupIdx; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *result = &m_ActiveSounds[i];
            if ((result->flags & SOUND_FLAG_USED)
                && result->sample_id == sample_id && result->pos == pos) {
                result->flags |= SOUND_FLAG_RESTARTED;
                return result;
            } else if (result->flags == SOUND_FLAG_UNUSED) {
                last_free_sound = result;
            }
        }
        return last_free_sound;
    }

    case SOUND_MODE_AMBIENT:
        for (int32_t i = 0; i < M_MAX_AMBIENT_FX; i++) {
            if (m_AmbientLookup[i] == sample_id) {
                M_ACTIVE_SOUND *result = &m_ActiveSounds[i];
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

static void M_UpdateActiveSoundParams(M_ACTIVE_SOUND *const sound)
{
    const SAMPLE_INFO *const info = Sound_GetSampleInfo(sound->sample_id);

    const int32_t x = sound->pos->x - g_Camera.target.x;
    const int32_t y = sound->pos->y - g_Camera.target.y;
    const int32_t z = sound->pos->z - g_Camera.target.z;
    if (ABS(x) > M_SOUND_RADIUS || ABS(y) > M_SOUND_RADIUS
        || ABS(z) > M_SOUND_RADIUS) {
        sound->volume = 0;
        return;
    }

    const uint32_t distance = SQUARE(x) + SQUARE(y) + SQUARE(z);
    int32_t volume =
        info->volume - Math_Sqrt(distance) * M_SOUND_RANGE_MULT_CONSTANT;
    if (volume < 0) {
        sound->volume = 0;
        return;
    }

    CLAMPG(volume, M_SOUND_MAX_VOLUME);

    sound->volume = volume;

    if (!distance || (info->flags & SAMPLE_FLAG_NO_PAN)) {
        sound->pan = 0;
        return;
    }

    int16_t angle = Math_Atan(
        sound->pos->z - g_LaraItem->pos.z, sound->pos->x - g_LaraItem->pos.x);
    angle -= g_LaraItem->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    sound->pan = angle;
}

static void M_ClearActiveSound(M_ACTIVE_SOUND *const sound)
{
    sound->handle = AUDIO_NO_SOUND;
    sound->pos = nullptr;
    sound->flags = SOUND_FLAG_UNUSED;
    sound->volume = 0;
    sound->pan = 0;
    sound->loudness = M_SOUND_NOT_AUDIBLE;
    sound->sample_id = SFX_INVALID;
}

static void M_ClearActiveSoundHandles(const M_ACTIVE_SOUND *const sound)
{
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const rsound = &m_ActiveSounds[i];
        if (rsound != sound && rsound->handle == sound->handle) {
            rsound->handle = AUDIO_NO_SOUND;
        }
    }
}

static void M_ResetAmbientLoudness(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < m_AmbientLookupIdx; i++) {
        M_ACTIVE_SOUND *sound = &m_ActiveSounds[i];
        sound->loudness = M_SOUND_NOT_AUDIBLE;
    }
}

bool Sound_Init(void)
{
    return Sound_InitialiseCommon();
}

void Sound_UpdateEffects(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (!(sound->flags & SOUND_FLAG_USED)) {
            continue;
        }

        if (sound->flags & SOUND_FLAG_AMBIENT) {
            if (sound->loudness != (uint32_t)M_SOUND_NOT_AUDIBLE
                && sound->handle != AUDIO_NO_SOUND) {
                Audio_Sample_SetPan(
                    sound->handle, Sound_ConvertPanToDecibel(sound->pan));
                Audio_Sample_SetVolume(
                    sound->handle, Sound_ConvertVolumeToDecibel(sound->volume));
            } else {
                if (sound->handle != AUDIO_NO_SOUND) {
                    Audio_Sample_Close(sound->handle);
                }
                M_ClearActiveSound(sound);
            }
        } else if (Audio_Sample_IsPlaying(sound->handle)) {
            if (sound->pos != nullptr) {
                M_UpdateActiveSoundParams(sound);
                if (sound->volume > 0 && sound->handle != AUDIO_NO_SOUND) {
                    Audio_Sample_SetPan(
                        sound->handle, Sound_ConvertPanToDecibel(sound->pan));
                    Audio_Sample_SetVolume(
                        sound->handle,
                        Sound_ConvertVolumeToDecibel(sound->volume));
                } else {
                    if (sound->handle != AUDIO_NO_SOUND) {
                        Audio_Sample_Close(sound->handle);
                    }
                    M_ClearActiveSound(sound);
                }
            }
        } else {
            M_ClearActiveSound(sound);
        }
    }
}

bool Sound_Effect(
    const SAMPLE_ID sample_id, const XYZ_32 *const pos, const uint32_t flags)
{
    if (!Sound_IsInitialised()) {
        return false;
    }

    if (flags != SPM_ALWAYS
        && (flags & SPM_UNDERWATER)
            != (Room_Get(g_Camera.pos.room_num)->flags & RF_UNDERWATER)) {
        return false;
    }

    const SAMPLE_INFO *const info = Sound_GetSampleInfo(sample_id);
    if (info == nullptr) {
        return false;
    }

    if (info->randomness && Random_GetDraw() > (int32_t)info->randomness) {
        return false;
    }

    const M_SOUND_MODE mode = info->flags & SOUND_MODE_MASK;
    int32_t pan = 0x7FFF;
    uint32_t distance;
    if (pos != nullptr) {
        int32_t x = pos->x - g_Camera.target.x;
        int32_t y = pos->y - g_Camera.target.y;
        int32_t z = pos->z - g_Camera.target.z;
        if (ABS(x) > M_SOUND_RADIUS || ABS(y) > M_SOUND_RADIUS
            || ABS(z) > M_SOUND_RADIUS) {
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

    int32_t volume = info->volume - distance * M_SOUND_RANGE_MULT_CONSTANT;
    if (info->flags & SAMPLE_FLAG_VOLUME_WIBBLE) {
        volume -= Random_GetDraw() * M_SOUND_MAX_VOLUME_CHANGE >> 15;
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
        pitch += ((Random_GetDraw() * M_SOUND_MAX_PITCH_CHANGE) / 0x4000)
            - M_SOUND_MAX_PITCH_CHANGE;
    }

    int32_t vars = (info->flags >> 2) & 15;
    int32_t sfx_id = info->number;
    if (vars != 1) {
        sfx_id += (Random_GetDraw() * vars) / 0x8000;
    }

    CLAMPG(volume, M_SOUND_MAX_VOLUME);

    switch (mode) {
    default:
        break;

    case SOUND_MODE_WAIT: {
        M_ACTIVE_SOUND *const sound = M_GetActiveSound(sample_id, 0, pos, mode);
        if (sound == nullptr) {
            return false;
        }
        if (sound->flags & SOUND_FLAG_RESTARTED) {
            sound->flags &= ~SOUND_FLAG_RESTARTED;
            return true;
        }
        sound->handle = Audio_Sample_Play(
            sfx_id, Sound_ConvertVolumeToDecibel(volume), M_CalcPitch(pitch),
            Sound_ConvertPanToDecibel(pan), false);
        if (sound->handle == AUDIO_NO_SOUND) {
            return false;
        }
        M_ClearActiveSoundHandles(sound);
        sound->flags = SOUND_FLAG_USED;
        sound->sample_id = sample_id;
        sound->pos = pos;
        return true;
    }

    case SOUND_MODE_RESTART: {
        M_ACTIVE_SOUND *const sound = M_GetActiveSound(sample_id, 0, pos, mode);
        if (sound == nullptr) {
            return false;
        }
        if (sound->flags & SOUND_FLAG_RESTARTED) {
            Audio_Sample_Close(sound->handle);
            sound->handle = Audio_Sample_Play(
                sfx_id, Sound_ConvertVolumeToDecibel(volume),
                M_CalcPitch(pitch), Sound_ConvertPanToDecibel(pan), false);

            M_ClearActiveSoundHandles(sound);
            return true;
        }
        sound->handle = Audio_Sample_Play(
            sfx_id, Sound_ConvertVolumeToDecibel(volume), M_CalcPitch(pitch),
            Sound_ConvertPanToDecibel(pan), false);
        if (sound->handle == AUDIO_NO_SOUND) {
            return false;
        }
        M_ClearActiveSoundHandles(sound);
        sound->flags = SOUND_FLAG_USED;
        sound->sample_id = sample_id;
        sound->pos = pos;
        return true;
    }

    case SOUND_MODE_AMBIENT: {
        uint32_t loudness = distance;
        M_ACTIVE_SOUND *const sound =
            M_GetActiveSound(sample_id, loudness, pos, mode);
        if (sound == nullptr) {
            return false;
        }
        sound->pos = pos;
        if (sound->flags & SOUND_FLAG_AMBIENT) {
            if (volume > 0) {
                sound->loudness = loudness;
                sound->pan = pan;
                sound->volume = volume;
            } else {
                sound->loudness = M_SOUND_NOT_AUDIBLE;
                sound->volume = 0;
            }
            return true;
        }

        if (volume > 0) {
            sound->handle = Audio_Sample_Play(
                sfx_id, Sound_ConvertVolumeToDecibel(volume),
                M_CalcPitch(pitch), Sound_ConvertPanToDecibel(pan), true);
            if (sound->handle == AUDIO_NO_SOUND) {
                M_ClearActiveSound(sound);
                return false;
            }
            M_ClearActiveSoundHandles(sound);
            sound->loudness = loudness;
            sound->sample_id = sample_id;
            sound->pan = pan;
            sound->volume = volume;
            sound->flags |= SOUND_FLAG_AMBIENT | SOUND_FLAG_USED;
            sound->pos = pos;
            return true;
        }

        sound->loudness = M_SOUND_NOT_AUDIBLE;
        return true;
    }
    }

    return false;
}

void Sound_StopEffect(const SAMPLE_ID sample_id)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if ((sound->flags & SOUND_FLAG_USED) != 0
            && sound->sample_id == sample_id
            && Audio_Sample_IsPlaying(sound->handle)) {
            Audio_Sample_Close(sound->handle);
            M_ClearActiveSound(sound);
            break;
        }
    }
}

void Sound_ResetEffects(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ClearActiveSound(&m_ActiveSounds[i]);
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

        const M_SOUND_MODE mode = info->flags & SOUND_MODE_MASK;
        if (mode == SOUND_MODE_AMBIENT) {
            if (m_AmbientLookupIdx >= M_MAX_AMBIENT_FX) {
                LOG_ERROR("Ran out of ambient effect slots");
                return;
            }
            m_AmbientLookup[m_AmbientLookupIdx] = i;
            m_AmbientLookupIdx++;
        }
    }
}

void Sound_StopAmbientSounds(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < m_AmbientLookupIdx; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (Audio_Sample_IsPlaying(sound->handle)) {
            Audio_Sample_Close(sound->handle);
            M_ClearActiveSound(sound);
        }
    }
}

void Sound_Reset(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }
    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();
}

void Sound_StopAll(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }
    Audio_Sample_CloseAll();
}

void Sound_ResetAmbient(void)
{
    M_ResetAmbientLoudness();
    Sound_ResetSources();
}
