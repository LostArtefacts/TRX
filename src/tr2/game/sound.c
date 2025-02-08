#include "game/sound.h"

#include "game/camera.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/engine/audio.h>
#include <libtrx/game/math.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

#include <math.h>

typedef enum {
    SOUND_MODE_NORMAL = 0,
    SOUND_MODE_WAIT = 1,
    SOUND_MODE_RESTART = 2,
    SOUND_MODE_LOOPED = 3,
    SOUND_MODE_MASK = 3,
} SOUND_MODE;

typedef struct {
    int32_t volume;
    int32_t pan;
    SOUND_EFFECT_ID effect_num;
    int32_t pitch;
    int32_t handle;
} SOUND_SLOT;

typedef enum {
    // clang-format off
    SF_NO_PAN        = 1 << 12, // = 0x1000 = 4096
    SF_PITCH_WIBBLE  = 1 << 13, // = 0x2000 = 8192
    SF_VOLUME_WIBBLE = 1 << 14, // = 0x4000 = 16384
    // clang-format on
} SAMPLE_FLAG;

#define SOUND_RANGE 10
#define SOUND_RADIUS (SOUND_RANGE * WALL_L) // = 0x2800 = 10240
#define SOUND_RADIUS_SQRD SQUARE(SOUND_RADIUS) // = 0x6400000

#define SOUND_MAX_SLOTS 32
#define SOUND_MAX_VOLUME (SOUND_RADIUS - 1)
#define SOUND_MAX_VOLUME_CHANGE 0x2000
#define SOUND_MAX_PITCH_CHANGE 6000

#define SOUND_MAXVOL_RANGE 1
#define SOUND_MAXVOL_RADIUS (SOUND_MAXVOL_RANGE * WALL_L) // = 0x400 = 1024
#define SOUND_MAXVOL_RADIUS_SQRD SQUARE(SOUND_MAXVOL_RADIUS) // = 0x100000

#define DECIBEL_LUT_SIZE 512

static float m_MasterVolume = 0.0f;
static int32_t m_DecibelLUT[DECIBEL_LUT_SIZE] = {};
static SOUND_SLOT m_SoundSlots[SOUND_MAX_SLOTS] = {};

static int32_t M_ConvertVolumeToDecibel(int32_t volume);
static int32_t M_ConvertPanToDecibel(uint16_t pan);
static float M_ConvertPitch(float pitch);
static int32_t M_Play(
    int32_t track_id, int32_t volume, float pitch, int32_t pan, bool is_looped);

static void M_ClearSlot(SOUND_SLOT *const slot);
static void M_ClearAllSlots(void);
static void M_CloseSlot(SOUND_SLOT *const slot);
static void M_UpdateSlot(SOUND_SLOT *const slot);

static int32_t M_ConvertVolumeToDecibel(const int32_t volume)
{
    const double adjusted_volume = m_MasterVolume * volume;
    const double scaler = 0x1.p-21; // 2.0e-21
    return (adjusted_volume * scaler - 1.0) * 5000.0;
}

static int32_t M_ConvertPanToDecibel(const uint16_t pan)
{
    const int32_t result = sin((pan / 32767.0) * M_PI) * (DECIBEL_LUT_SIZE / 2);
    if (result > 0) {
        return -m_DecibelLUT[DECIBEL_LUT_SIZE - result];
    } else if (result < 0) {
        return m_DecibelLUT[DECIBEL_LUT_SIZE + result];
    } else {
        return 0;
    }
}

static float M_ConvertPitch(const float pitch)
{
    return pitch / 0x10000.p0;
}

static int32_t M_Play(
    const int32_t sample_num, const int32_t volume, const float pitch,
    const int32_t pan, const bool is_looped)
{
    const int32_t handle = Audio_Sample_Play(
        sample_num, M_ConvertVolumeToDecibel(volume), M_ConvertPitch(pitch),
        M_ConvertPanToDecibel(pan), is_looped);
    return handle;
}

static void M_ClearAllSlots(void)
{
    for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
        SOUND_SLOT *const slot = &m_SoundSlots[i];
        M_ClearSlot(slot);
    }
}

static void M_ClearSlot(SOUND_SLOT *const slot)
{
    slot->effect_num = SFX_INVALID;
    slot->handle = AUDIO_NO_SOUND;
}

static void M_CloseSlot(SOUND_SLOT *const slot)
{
    Audio_Sample_Close(slot->handle);
    M_ClearSlot(slot);
}

static void M_UpdateSlot(SOUND_SLOT *const slot)
{
    Audio_Sample_SetPan(slot->handle, M_ConvertPanToDecibel(slot->pan));
    Audio_Sample_SetPitch(slot->handle, M_ConvertPitch(slot->pitch));
    Audio_Sample_SetVolume(
        slot->handle, M_ConvertVolumeToDecibel(slot->volume));
}

void Sound_Init(void)
{
    m_DecibelLUT[0] = -10000;
    for (int32_t i = 1; i < DECIBEL_LUT_SIZE; i++) {
        m_DecibelLUT[i] = (log2(1.0 / DECIBEL_LUT_SIZE) - log2(1.0 / i)) * 1000;
    }

    if (!Audio_Init()) {
        LOG_ERROR("Failed to initialize libtrx sound system");
        return;
    }

    Sound_SetMasterVolume(g_Config.audio.sound_volume);
    M_ClearAllSlots();
}

void Sound_Shutdown(void)
{
    Audio_Shutdown();
    M_ClearAllSlots();
}

void Sound_SetMasterVolume(int32_t volume)
{
    m_MasterVolume = volume * 64.0f / 10.0f;
}

void Sound_UpdateEffects(void)
{
    Sound_ResetSources();
}

bool Sound_Effect(
    const SOUND_EFFECT_ID sample_id, const XYZ_32 *const pos,
    const uint32_t flags)
{
    if (flags != SPM_ALWAYS
        && ((flags & SPM_UNDERWATER)
            != (Room_Get(g_Camera.pos.room_num)->flags & RF_UNDERWATER))) {
        return false;
    }

    SAMPLE_INFO *const info = Sound_GetSampleInfo(sample_id);
    if (info == nullptr) {
        return false;
    }

    if (info->randomness && (Random_GetDraw() > info->randomness)) {
        return false;
    }

    uint32_t distance = 0;
    int32_t pan = 0;
    if (pos) {
        const int32_t dx = pos->x - g_Camera.mic_pos.x;
        const int32_t dy = pos->y - g_Camera.mic_pos.y;
        const int32_t dz = pos->z - g_Camera.mic_pos.z;
        if (ABS(dx) > SOUND_RADIUS || ABS(dy) > SOUND_RADIUS
            || ABS(dz) > SOUND_RADIUS) {
            return false;
        }
        distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (distance > SOUND_RADIUS_SQRD) {
            return false;
        } else if (distance < SOUND_MAXVOL_RADIUS_SQRD) {
            distance = 0;
        } else {
            distance = Math_Sqrt(distance) - SOUND_MAXVOL_RADIUS;
        }
        if (!(info->flags & SF_NO_PAN)) {
            pan = (int16_t)Math_Atan(dz, dx) - g_Camera.actual_angle;
        }
    }

    int32_t volume = info->volume;
    if (info->flags & SF_VOLUME_WIBBLE) {
        volume -= Random_GetDraw() * SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    const int32_t attenuation =
        SQUARE(distance) / (SOUND_RADIUS_SQRD / 0x10000);
    volume = (volume * (0x10000 - attenuation)) / 0x10000;

    if (volume <= 0) {
        return false;
    }

    int32_t pitch = (flags & SPM_PITCH) != 0 ? (flags >> 8) & 0xFFFFFF
                                             : SOUND_DEFAULT_PITCH;
    if (info->flags & SF_PITCH_WIBBLE) {
        pitch += ((Random_GetDraw() * SOUND_MAX_PITCH_CHANGE) / 0x4000)
            - SOUND_MAX_PITCH_CHANGE;
    }

    if (info->number < 0) {
        return false;
    }

    const SOUND_MODE mode = info->flags & SOUND_MODE_MASK;
    const int32_t num_samples = (info->flags >> 2) & 0xF;
    const int32_t track_id = num_samples == 1
        ? info->number
        : info->number + (int32_t)((num_samples * Random_GetDraw()) / 0x8000);

    switch (mode) {
    case SOUND_MODE_NORMAL:
        break;

    case SOUND_MODE_WAIT:
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->effect_num == sample_id) {
                if (Audio_Sample_IsPlaying(i)) {
                    return true;
                }
                M_ClearSlot(slot);
            }
        }
        break;

    case SOUND_MODE_RESTART:
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->effect_num == sample_id) {
                M_CloseSlot(slot);
                break;
            }
        }
        break;

    case SOUND_MODE_LOOPED:
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->effect_num == sample_id) {
                if (volume > slot->volume) {
                    slot->volume = volume;
                    slot->pan = pan;
                    slot->pitch = pitch;
                }
                return true;
            }
        }
        break;
    }

    const bool is_looped = mode == SOUND_MODE_LOOPED;
    int32_t handle = M_Play(track_id, volume, pitch, pan, is_looped);

    if (handle == AUDIO_NO_SOUND) {
        int32_t min_volume = 0x8000;
        int32_t min_slot = -1;
        for (int32_t i = 1; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->effect_num >= 0 && slot->volume < min_volume) {
                min_volume = slot->volume;
                min_slot = i;
            }
        }

        if (min_slot >= 0 && volume >= min_volume) {
            SOUND_SLOT *const slot = &m_SoundSlots[min_slot];
            M_CloseSlot(slot);
            handle = M_Play(track_id, volume, pitch, pan, is_looped);
        }
    }

    if (handle == AUDIO_NO_SOUND) {
        info->number = -1;
        return false;
    }

    int32_t free_slot = -1;
    for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
        SOUND_SLOT *const slot = &m_SoundSlots[i];
        if (slot->effect_num < 0) {
            free_slot = i;
            break;
        }
    }

    if (free_slot != -1) {
        SOUND_SLOT *const slot = &m_SoundSlots[free_slot];
        slot->volume = volume;
        slot->pan = pan;
        slot->pitch = pitch;
        slot->effect_num = sample_id;
        slot->handle = handle;
    }
    return true;
}

void Sound_StopEffect(const SOUND_EFFECT_ID sample_id)
{
    for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
        SOUND_SLOT *const slot = &m_SoundSlots[i];
        if (slot->effect_num == sample_id) {
            M_CloseSlot(slot);
        }
    }
}

void Sound_StopAll(void)
{
    Audio_Sample_CloseAll();
    M_ClearAllSlots();
}

void Sound_EndScene(void)
{
    for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
        SOUND_SLOT *const slot = &m_SoundSlots[i];
        const SAMPLE_INFO *const info = Sound_GetSampleInfo(slot->effect_num);
        if (info == nullptr) {
            continue;
        }

        if ((info->flags & SOUND_MODE_MASK) == SOUND_MODE_LOOPED) {
            if (slot->volume == 0) {
                M_CloseSlot(slot);
            } else {
                M_UpdateSlot(slot);
                slot->volume = 0;
            }
        } else if (!Audio_Sample_IsPlaying(slot->handle)) {
            M_ClearSlot(slot);
        }
    }
}

int32_t Sound_GetMinVolume(void)
{
    return 0;
}

int32_t Sound_GetMaxVolume(void)
{
    return 10;
}
