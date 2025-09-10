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
#define M_SOUND_RANGE_MULT_CONSTANT 4
#define M_SOUND_FAR_RANGE (8 * WALL_L)
#define M_SOUND_MAX_VOLUME                                                     \
    ((M_SOUND_FAR_RANGE * M_SOUND_RANGE_MULT_CONSTANT) - 1)
#define M_SOUND_MAX_VOLUME_CHANGE 0x2000

typedef struct {
    int32_t handle;
    SAMPLE_ID sample_id;
    const SAMPLE_INFO *sample;
    int16_t volume;
    int16_t pan;
    int32_t pitch;

    int32_t distance;
    const XYZ_32 *pos;
} M_ACTIVE_SOUND;

static M_ACTIVE_SOUND m_ActiveSounds[M_MAX_ACTIVE_SOUNDS] = {};

static float M_ConvertPitch(int32_t pitch);

static M_ACTIVE_SOUND *M_SelectUnusedSound(void);
static M_ACTIVE_SOUND *M_SelectUsedSound(const SAMPLE_ID sample_id);
static M_ACTIVE_SOUND *M_SelectUsedSoundWithPos(
    SAMPLE_ID sample_id, const XYZ_32 *pos);

static void M_ClearAllActiveSounds(void);
static void M_ClearActiveSound(M_ACTIVE_SOUND *sound);
static void M_CloseActiveSound(M_ACTIVE_SOUND *sound);
static void M_ClearActiveSoundHandles(const M_ACTIVE_SOUND *sound);

static void M_SyncActiveSoundHandle(M_ACTIVE_SOUND *sound);
static void M_UpdateActiveSoundParams(M_ACTIVE_SOUND *sound);

static float M_ConvertPitch(const int32_t pitch)
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

static M_ACTIVE_SOUND *M_SelectUsedSoundWithPos(
    const SAMPLE_ID sample_id, const XYZ_32 *const pos)
{
    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const result = &m_ActiveSounds[i];
        if (result->sample_id == sample_id && result->pos == pos) {
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
    sound->volume = volume;
    sound->pan = pan;
    sound->pitch = pitch;
    sound->sample_id = sample_id;
    sound->handle = handle;
    sound->distance = distance;
    sound->pos = pos;
    M_ClearActiveSoundHandles(sound);
    return true;
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

static void M_SyncActiveSoundHandle(M_ACTIVE_SOUND *const sound)
{
    Audio_Sample_SetPan(sound->handle, Sound_ConvertPanToDecibel(sound->pan));
    Audio_Sample_SetPitch(sound->handle, M_ConvertPitch(sound->pitch));
    Audio_Sample_SetVolume(
        sound->handle, Sound_ConvertVolumeToDecibel(sound->volume));
}

static void M_UpdateActiveSoundParams(M_ACTIVE_SOUND *const sound)
{
    const SAMPLE_INFO *const info = Sound_GetSampleInfo(sound->sample_id);

    const int32_t x = sound->pos->x - g_Camera.target.x;
    const int32_t y = sound->pos->y - g_Camera.target.y;
    const int32_t z = sound->pos->z - g_Camera.target.z;
    if (ABS(x) > M_SOUND_FAR_RANGE || ABS(y) > M_SOUND_FAR_RANGE
        || ABS(z) > M_SOUND_FAR_RANGE) {
        sound->volume = 0;
        return;
    }

    const int32_t distance = SQUARE(x) + SQUARE(y) + SQUARE(z);
    int32_t volume =
        info->volume - Math_Sqrt(distance) * M_SOUND_RANGE_MULT_CONSTANT;
    if (volume < 0) {
        sound->volume = 0;
        return;
    }

    CLAMPG(volume, M_SOUND_MAX_VOLUME);

    sound->volume = volume;

    if (distance == 0 || info->flags.no_pan) {
        sound->pan = 0;
        return;
    }

    int16_t angle = Math_Atan(
        sound->pos->z - g_LaraItem->pos.z, sound->pos->x - g_LaraItem->pos.x);
    angle -= g_LaraItem->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    sound->pan = angle;
}

bool Sound_Init(void)
{
    return Sound_InitialiseCommon();
}

void Sound_ResetAmbient(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }
    Sound_ResetSources();

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample != nullptr
            && sound->sample->mode == SAMPLE_MODE_LOOPED) {
            sound->distance = -1;
        }
    }
}

void Sound_UpdateEffects(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample == nullptr) {
            continue;
        }

        const SAMPLE_INFO *const info = Sound_GetSampleInfo(sound->sample_id);
        if (info->mode == SAMPLE_MODE_LOOPED) {
            if (sound->volume <= 0) {
                M_ClearActiveSound(sound);
            } else {
                M_SyncActiveSoundHandle(sound);
            }
        } else if (!Audio_Sample_IsPlaying(sound->handle)) {
            M_ClearActiveSound(sound);
        } else if (sound->pos != nullptr) {
            M_UpdateActiveSoundParams(sound);
            if (sound->volume <= 0) {
                M_CloseActiveSound(sound);
            } else {
                M_SyncActiveSoundHandle(sound);
            }
        }
    }
}

int32_t Sound_GetDistance(const XYZ_32 *const pos)
{
    if (pos == nullptr) {
        return 0;
    }
    const int32_t dx = pos->x - g_Camera.target.x;
    const int32_t dy = pos->y - g_Camera.target.y;
    const int32_t dz = pos->z - g_Camera.target.z;
    if (ABS(dx) > M_SOUND_FAR_RANGE || ABS(dy) > M_SOUND_FAR_RANGE
        || ABS(dz) > M_SOUND_FAR_RANGE) {
        return INT32_MAX;
    }
    const uint32_t distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
    return Math_Sqrt(distance);
}

int32_t Sound_GetPan(const SAMPLE_INFO *const sample, const XYZ_32 *const pos)
{
    if (pos == nullptr) {
        return 0;
    }
    const int32_t distance = Sound_GetDistance(pos);
    if (distance > 0 && !sample->flags.no_pan) {
        int16_t angle =
            Math_Atan(pos->z - g_LaraItem->pos.z, pos->x - g_LaraItem->pos.x);
        angle -= g_LaraItem->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
        return angle;
    }
    return 0;
}

int32_t Sound_GetVolume(
    const SAMPLE_INFO *const sample, const uint32_t distance)
{
    int32_t volume = sample->volume - distance * M_SOUND_RANGE_MULT_CONSTANT;
    if (sample->flags.randomize_volume) {
        volume -= Random_GetDraw() * M_SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    return volume;
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

    switch (info->mode) {
    case SAMPLE_MODE_NORMAL:
        M_ACTIVE_SOUND *const sound = M_SelectUnusedSound();
        if (sound == nullptr) {
            return false;
        }
        return M_Play(
            sound, info, sample_id, track_id, volume, pitch, pan, distance,
            pos);

    case SAMPLE_MODE_WAIT: {
        M_ACTIVE_SOUND *sound = M_SelectUsedSoundWithPos(sample_id, pos);
        if (sound == nullptr) {
            sound = M_SelectUnusedSound();
        }
        if (sound == nullptr) {
            return false;
        }
        if (Audio_Sample_IsPlaying(sound->handle)) {
            return true;
        }
        return M_Play(
            sound, info, sample_id, track_id, volume, pitch, pan, distance,
            pos);
    }

    case SAMPLE_MODE_RESTART: {
        M_ACTIVE_SOUND *sound = M_SelectUsedSound(sample_id);
        if (sound == nullptr) {
            sound = M_SelectUnusedSound();
        }
        if (sound == nullptr) {
            return false;
        }
        return M_Play(
            sound, info, sample_id, track_id, volume, pitch, pan, distance,
            pos);
    }

    case SAMPLE_MODE_LOOPED: {
        for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
            M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
            if (sound->sample != nullptr
                && sound->sample->mode == SAMPLE_MODE_LOOPED
                && sound->sample_id == sample_id) {
                if (sound->distance == -1 || distance < sound->distance) {
                    sound->distance = distance;
                    sound->volume = volume;
                    sound->pan = pan;
                    sound->pitch = pitch;
                    return true;
                }
                return false;
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

    return false;
}

void Sound_StopEffect(const SAMPLE_ID sample_id)
{
    if (!Sound_IsInitialised()) {
        return;
    }

    for (int32_t i = 0; i < M_MAX_ACTIVE_SOUNDS; i++) {
        M_ACTIVE_SOUND *const sound = &m_ActiveSounds[i];
        if (sound->sample_id == sample_id
            && Audio_Sample_IsPlaying(sound->handle)) {
            Audio_Sample_Close(sound->handle);
            M_ClearActiveSound(sound);
            break;
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
    M_ClearAllActiveSounds();
}
