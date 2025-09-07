#include "game/sound.h"

#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/engine/audio.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/math.h>
#include <libtrx/game/random.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

#include <math.h>

typedef struct {
    SAMPLE_ID sample_id;
    int32_t handle;
    int32_t volume;
    int32_t pan;
    int32_t pitch;
} M_ACTIVE_SOUND;

#define M_SOUND_RANGE 10
#define M_SOUND_RADIUS (M_SOUND_RANGE * WALL_L) // = 0x2800 = 10240
#define M_SOUND_RADIUS_SQRD SQUARE(M_SOUND_RADIUS) // = 0x6400000

#define M_MAX_ACTIVE_SOUNDS 32
// sample volume ranges from 0..32767
#define M_SOUND_MAX_VOLUME_CHANGE 0x2000

#define M_SOUND_MAXVOL_RANGE 1
#define M_SOUND_MAXVOL_RADIUS (M_SOUND_MAXVOL_RANGE * WALL_L) // = 0x400 = 1024
#define M_SOUND_MAXVOL_RADIUS_SQRD SQUARE(M_SOUND_MAXVOL_RADIUS) // = 0x100000

static M_ACTIVE_SOUND m_ActiveSounds[M_MAX_ACTIVE_SOUNDS] = {};

static float M_ConvertPitch(float pitch);

static void M_ClearAllActiveSounds(void);
static void M_ClearActiveSound(M_ACTIVE_SOUND *sound);
static void M_CloseActiveSound(M_ACTIVE_SOUND *sound);

static void M_UpdateActiveSound(M_ACTIVE_SOUND *sound);

static float M_ConvertPitch(const float pitch)
{
    return pitch / 0x10000.p0;
}

static void M_ClearAllActiveSounds(void)
{
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        M_ClearActiveSound(sound);
    }
}

static void M_ClearActiveSound(M_ACTIVE_SOUND *const sound)
{
    sound->sample_id = SFX_INVALID;
    sound->handle = AUDIO_NO_SOUND;
}

static void M_CloseActiveSound(M_ACTIVE_SOUND *const sound)
{
    Audio_Sample_Close(sound->handle);
    M_ClearActiveSound(sound);
}

static void M_UpdateActiveSound(M_ACTIVE_SOUND *const sound)
{
    Audio_Sample_SetPan(sound->handle, Sound_ConvertPanToDecibel(sound->pan));
    Audio_Sample_SetPitch(sound->handle, M_ConvertPitch(sound->pitch));
    Audio_Sample_SetVolume(
        sound->handle, Sound_ConvertVolumeToDecibel(sound->volume));
}

bool Sound_Init(void)
{
    const bool result = Sound_InitialiseCommon();
    if (result) {
        M_ClearAllActiveSounds();
    }
    return result;
}

void Sound_UpdateEffects(void)
{
    Sound_ResetSources();

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        const SAMPLE_INFO *const info = Sound_GetSampleInfo(sound->sample_id);
        if (info == nullptr) {
            continue;
        }

        if (info->mode == SAMPLE_MODE_LOOPED) {
            if (sound->volume == 0) {
                M_CloseActiveSound(sound);
            } else {
                M_UpdateActiveSound(sound);
                sound->volume = 0;
            }
        } else if (!Audio_Sample_IsPlaying(sound->handle)) {
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
        && ((flags & SPM_UNDERWATER)
            != (Room_Get(g_Camera.pos.room_num)->flags & RF_UNDERWATER))) {
        return false;
    }

    const SAMPLE_INFO *const info = Sound_GetSampleInfo(sample_id);
    if (info == nullptr || info->number < 0) {
        return false;
    }

    if (info->randomness && Random_GetDraw() > info->randomness) {
        return false;
    }

    uint32_t distance = 0;
    int32_t pan = 0;
    if (pos != nullptr) {
        const int32_t dx = pos->x - g_Camera.mic_pos.x;
        const int32_t dy = pos->y - g_Camera.mic_pos.y;
        const int32_t dz = pos->z - g_Camera.mic_pos.z;
        if (ABS(dx) > M_SOUND_RADIUS || ABS(dy) > M_SOUND_RADIUS
            || ABS(dz) > M_SOUND_RADIUS) {
            return false;
        }
        distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (distance > M_SOUND_RADIUS_SQRD) {
            return false;
        } else if (distance < M_SOUND_MAXVOL_RADIUS_SQRD) {
            distance = 0;
        } else {
            distance = Math_Sqrt(distance) - M_SOUND_MAXVOL_RADIUS;
        }
        if (!info->flags.no_pan) {
            pan = (int16_t)Math_Atan(dz, dx) - g_Camera.actual_angle;
        }
    }

    int32_t volume = info->volume;
    if (info->flags.randomize_volume) {
        volume -= Random_GetDraw() * M_SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    const int32_t attenuation =
        SQUARE(distance) / (M_SOUND_RADIUS_SQRD / 0x10000);
    volume = (volume * (0x10000 - attenuation)) / 0x10000;

    if (volume <= 0) {
        return false;
    }

    const int32_t pitch = Sound_GetPitch(info);

    const int32_t num_samples = info->flags.num_samples;
    const int32_t track_id = num_samples == 1
        ? info->number
        : info->number + ((num_samples * Random_GetDraw()) / 0x8000);

    switch (info->mode) {
    case SAMPLE_MODE_NORMAL:
        break;

    case SAMPLE_MODE_WAIT:
        for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
            if (sound->sample_id == sample_id) {
                if (Audio_Sample_IsPlaying(sound->handle)) {
                    return true;
                }
                M_ClearActiveSound(sound);
            }
        }
        break;

    case SAMPLE_MODE_RESTART:
        for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
            if (sound->sample_id == sample_id) {
                M_CloseActiveSound(sound);
                break;
            }
        }
        break;

    case SAMPLE_MODE_LOOPED:
        for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
            if (sound->sample_id == sample_id) {
                if (volume > sound->volume) {
                    sound->volume = volume;
                    sound->pan = pan;
                    sound->pitch = pitch;
                }
                return true;
            }
        }
        break;
    }

    int32_t free_sound_idx = -1;
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample_id < 0) {
            free_sound_idx = i;
            break;
        }
    }

    if (free_sound_idx == -1) {
        // No sound found - try to find the most quiet track, and use this one
        int32_t min_volume = INT32_MAX;
        for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
            if (sound->sample_id >= 0 && sound->volume < min_volume) {
                min_volume = sound->volume;
                free_sound_idx = i;
            }
        }

        if (free_sound_idx == -1) {
            // No sound found - give up
            return false;
        }
    }

    M_ACTIVE_SOUND *const sound = &m_ActiveSounds[free_sound_idx];
    M_CloseActiveSound(sound);

    const int32_t handle = Audio_Sample_Play(
        track_id, Sound_ConvertVolumeToDecibel(volume), M_ConvertPitch(pitch),
        Sound_ConvertPanToDecibel(pan), info->mode == SAMPLE_MODE_LOOPED);
    if (handle != AUDIO_NO_SOUND) {
        sound->volume = volume;
        sound->pan = pan;
        sound->pitch = pitch;
        sound->sample_id = sample_id;
        sound->handle = handle;
    }

    return true;
}

void Sound_StopEffect(const SAMPLE_ID sample_id)
{
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample_id == sample_id) {
            M_CloseActiveSound(sound);
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
    M_ClearAllActiveSounds();
}

void Sound_StopAll(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }
    Audio_Sample_CloseAll();
    M_ClearAllActiveSounds();
}
