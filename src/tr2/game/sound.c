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
    const SAMPLE_INFO *sample;
    int32_t handle;
    int32_t volume;
    int32_t pitch;
    int32_t pan;

    int32_t distance;
    const XYZ_32 *pos;
} M_ACTIVE_SOUND;

#define M_MAX_ACTIVE_SOUNDS 32
// sample volume ranges from 0..32767
#define M_SOUND_MAX_VOLUME_CHANGE 0x2000

#define M_SOUND_CLOSE_RANGE WALL_L // = 0x400 = 1024
#define M_SOUND_FAR_RANGE (10 * WALL_L) // = 0x2800 = 10240

static M_ACTIVE_SOUND m_ActiveSounds[M_MAX_ACTIVE_SOUNDS] = {};

static float M_ConvertPitch(float pitch);

static M_ACTIVE_SOUND *M_SelectUnusedSound(void);
static M_ACTIVE_SOUND *M_SelectUsedSound(SAMPLE_ID sample_id);

static void M_ClearAllActiveSounds(void);
static void M_ClearActiveSound(M_ACTIVE_SOUND *sound);
static void M_CloseActiveSound(M_ACTIVE_SOUND *sound);
static void M_ClearActiveSoundHandles(const M_ACTIVE_SOUND *sound);

static bool M_Play(
    M_ACTIVE_SOUND *sound, const SAMPLE_INFO *info, int32_t sample_id,
    int32_t track_id, int32_t volume, int32_t pitch, int32_t pan,
    int32_t distance, const XYZ_32 *pos);

static void M_SyncActiveSoundHandle(M_ACTIVE_SOUND *sound);

static float M_ConvertPitch(const float pitch)
{
    return pitch / 0x10000.p0;
}

static M_ACTIVE_SOUND *M_SelectUnusedSound(void)
{
    // Try to get an unused slot
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample == nullptr) {
            return sound;
        }
    }

    // No sound found - try to find the most quiet track, and use this one
    M_ACTIVE_SOUND *best_sound = nullptr;
    int32_t min_volume = INT32_MAX;
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample != nullptr && sound->volume < min_volume) {
            min_volume = sound->volume;
            best_sound = sound;
        }
    }

    return best_sound;
}

static M_ACTIVE_SOUND *M_SelectUsedSound(const SAMPLE_ID sample_id)
{
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const result = &m_ActiveSounds[i];
        if (result->sample_id == sample_id) {
            return result;
        }
    }
    return nullptr;
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
    sound->sample = nullptr;
    sound->sample_id = SFX_INVALID;
    sound->handle = AUDIO_NO_SOUND;
}

static void M_CloseActiveSound(M_ACTIVE_SOUND *const sound)
{
    Audio_Sample_Close(sound->handle);
    M_ClearActiveSound(sound);
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

static bool M_Play(
    M_ACTIVE_SOUND *const sound, const SAMPLE_INFO *const info,
    const int32_t sample_id, const int32_t track_id, const int32_t volume,
    const int32_t pitch, const int32_t pan, const int32_t distance,
    const XYZ_32 *const pos)
{
    M_CloseActiveSound(sound);
    const int32_t handle = Audio_Sample_Play(
        track_id, Sound_ConvertVolumeToDecibel(volume), M_ConvertPitch(pitch),
        Sound_ConvertPanToDecibel(pan), info->mode == SAMPLE_MODE_LOOPED);
    if (handle == AUDIO_NO_SOUND) {
        return false;
    }
    sound->sample = info;
    sound->sample_id = sample_id;
    sound->handle = handle;
    sound->volume = volume;
    sound->pitch = pitch;
    sound->pan = pan;
    sound->distance = distance;
    sound->pos = pos;
    M_ClearActiveSoundHandles(sound);
    return true;
}

static void M_SyncActiveSoundHandle(M_ACTIVE_SOUND *const sound)
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

void Sound_ResetAmbient(void)
{
    Sound_ResetSources();
}

void Sound_UpdateEffects(void)
{
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
                M_SyncActiveSoundHandle(sound);
                sound->volume = 0;
            }
        } else if (!Audio_Sample_IsPlaying(sound->handle)) {
            M_ClearActiveSound(sound);
        }
    }
}

uint32_t Sound_GetDistance(const XYZ_32 *const pos)
{
    if (pos == nullptr) {
        return 0;
    }
    const int32_t dx = pos->x - g_Camera.mic_pos.x;
    const int32_t dy = pos->y - g_Camera.mic_pos.y;
    const int32_t dz = pos->z - g_Camera.mic_pos.z;
    if (ABS(dx) > M_SOUND_FAR_RANGE || ABS(dy) > M_SOUND_FAR_RANGE
        || ABS(dz) > M_SOUND_FAR_RANGE) {
        return INT32_MAX;
    }
    uint32_t distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
    if (distance > SQUARE(M_SOUND_FAR_RANGE)) {
        return INT32_MAX;
    } else if (distance < SQUARE(M_SOUND_CLOSE_RANGE)) {
        distance = 0;
    } else {
        distance = Math_Sqrt(distance) - M_SOUND_CLOSE_RANGE;
    }
    return distance;
}

int32_t Sound_GetPan(const SAMPLE_INFO *const sample, const XYZ_32 *const pos)
{
    if (pos == nullptr) {
        return 0;
    }
    const int32_t distance = Sound_GetDistance(pos);
    if (distance > 0 && !sample->flags.no_pan) {
        const int32_t dx = pos->x - g_Camera.mic_pos.x;
        const int32_t dz = pos->z - g_Camera.mic_pos.z;
        return (int16_t)Math_Atan(dz, dx) - g_Camera.actual_angle;
    }
    return 0;
}

int32_t Sound_GetVolume(const SAMPLE_INFO *const sample, const int32_t distance)
{
    int32_t volume = sample->volume;
    if (sample->flags.randomize_volume) {
        volume -= Random_GetDraw() * M_SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    const int32_t attenuation =
        SQUARE(distance) / (SQUARE(M_SOUND_FAR_RANGE) / 0x10000);
    return (volume * (0x10000 - attenuation)) / 0x10000;
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

    const int32_t distance = Sound_GetDistance(pos);
    if (distance == INT32_MAX) {
        return false;
    }

    const int32_t pan = Sound_GetPan(info, pos);
    const int32_t volume = Sound_GetVolume(info, distance);
    if (volume <= 0) {
        return false;
    }

    const int32_t pitch = Sound_GetPitch(info);
    const int32_t num_samples = info->flags.num_samples;
    const int32_t track_id = num_samples == 1
        ? info->number
        : info->number + ((num_samples * Random_GetDraw()) / 0x8000);

    M_ACTIVE_SOUND *sound = nullptr;
    switch (info->mode) {
    case SAMPLE_MODE_NORMAL:
        sound = M_SelectUnusedSound();
        break;

    case SAMPLE_MODE_WAIT:
        sound = M_SelectUsedSound(sample_id);
        if (sound != nullptr && Audio_Sample_IsPlaying(sound->handle)) {
            return true;
        }
        if (sound == nullptr) {
            sound = M_SelectUnusedSound();
        }
        break;

    case SAMPLE_MODE_RESTART:
        sound = M_SelectUsedSound(sample_id);
        if (sound == nullptr) {
            sound = M_SelectUnusedSound();
        }
        break;

    case SAMPLE_MODE_LOOPED: {
        for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
            if (sound->sample_id == sample_id) {
                if (volume > sound->volume) {
                    sound->distance = distance;
                    sound->volume = volume;
                    sound->pan = pan;
                    sound->pitch = pitch;
                }
                return true;
            }
        }
        M_ACTIVE_SOUND *const sound = M_SelectUnusedSound();
        if (sound == nullptr) {
            return false;
        }
        return M_Play(
            sound, info, sample_id, track_id, volume, pitch, pan, distance,
            pos);
    }
    }

    if (sound == nullptr) {
        return false;
    }
    return M_Play(
        sound, info, sample_id, track_id, volume, pitch, pan, distance, pos);
}

void Sound_StopEffect(const SAMPLE_ID sample_id)
{
    if (!Sound_IsInitialised()) {
        return;
    }
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
