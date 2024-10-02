#include "game/sound.h"

#include "game/math.h"
#include "game/random.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/engine/audio.h>
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
    int32_t sample_num;
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

#define SOUND_DEFAULT_PITCH 0x10000

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

static int32_t m_DecibelLUT[DECIBEL_LUT_SIZE] = { 0 };
static SOUND_SLOT m_SoundSlots[SOUND_MAX_SLOTS] = { 0 };

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
    const double adjusted_volume = g_MasterVolume * volume;
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
    slot->sample_num = -1;
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

void __cdecl Sound_Init(void)
{
    m_DecibelLUT[0] = -10000;
    for (int32_t i = 1; i < DECIBEL_LUT_SIZE; i++) {
        m_DecibelLUT[i] = (log2(1.0 / DECIBEL_LUT_SIZE) - log2(1.0 / i)) * 1000;
    }

    if (!Audio_Init()) {
        LOG_ERROR("Failed to initialize libtrx sound system");
        return;
    }

    Sound_SetMasterVolume(32);
    M_ClearAllSlots();
    g_SoundIsActive = true;
}

void __cdecl Sound_Shutdown(void)
{
    if (!g_SoundIsActive) {
        return;
    }

    Audio_Shutdown();
    M_ClearAllSlots();
}

void __cdecl Sound_SetMasterVolume(int32_t volume)
{
    g_MasterVolume = volume;
}

void __cdecl Sound_UpdateEffects(void)
{
    for (int32_t i = 0; i < g_SoundEffectCount; i++) {
        OBJECT_VECTOR *sound = &g_SoundEffects[i];
        if ((g_FlipStatus && (sound->flags & SF_FLIP))
            || (!g_FlipStatus && (sound->flags & SF_UNFLIP))) {
            Sound_Effect(
                sound->data, (XYZ_32 *)sound,
                SPM_NORMAL); // TODO: use proper pointer for this
        }
    }

    if (g_FlipEffect != -1) {
        g_EffectRoutines[g_FlipEffect](NULL);
    }

    Sound_EndScene();
}

void __cdecl Sound_Effect(
    const SOUND_EFFECT_ID sample_id, const XYZ_32 *const pos,
    const uint32_t flags)
{
    if (!g_SoundIsActive) {
        return;
    }

    if (flags != SPM_ALWAYS
        && ((flags & SPM_UNDERWATER)
            != (g_Rooms[g_Camera.pos.room_num].flags & RF_UNDERWATER))) {
        return;
    }

    const int32_t sample_num = g_SampleLUT[sample_id];
    if (sample_num == -1) {
        g_SampleLUT[sample_id] = -2;
        return;
    }
    if (sample_num == -2) {
        return;
    }

    SAMPLE_INFO *const s = &g_SampleInfos[sample_num];
    if (s->randomness && (Random_GetDraw() > s->randomness)) {
        return;
    }

    uint32_t distance = 0;
    int32_t pan = 0;
    if (pos) {
        const int32_t dx = pos->x - g_Camera.mic_pos.x;
        const int32_t dy = pos->y - g_Camera.mic_pos.y;
        const int32_t dz = pos->z - g_Camera.mic_pos.z;
        if (ABS(dx) > SOUND_RADIUS || ABS(dy) > SOUND_RADIUS
            || ABS(dz) > SOUND_RADIUS) {
            return;
        }
        distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (distance > SOUND_RADIUS_SQRD) {
            return;
        } else if (distance < SOUND_MAXVOL_RADIUS_SQRD) {
            distance = 0;
        } else {
            distance = Math_Sqrt(distance) - SOUND_MAXVOL_RADIUS;
        }
        if (!(s->flags & SF_NO_PAN)) {
            pan = (int16_t)Math_Atan(dz, dx) - g_Camera.actual_angle;
        }
    }

    int32_t volume = s->volume;
    if (s->flags & SF_VOLUME_WIBBLE) {
        volume -= Random_GetDraw() * SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    const int32_t attenuation =
        SQUARE(distance) / (SOUND_RADIUS_SQRD / 0x10000);
    volume = (volume * (0x10000 - attenuation)) / 0x10000;

    if (volume <= 0) {
        return;
    }

    int32_t pitch = (flags & SPM_PITCH) != 0 ? (flags >> 8) & 0xFFFFFF
                                             : SOUND_DEFAULT_PITCH;
    if (s->flags & SF_PITCH_WIBBLE) {
        pitch += ((Random_GetDraw() * SOUND_MAX_PITCH_CHANGE) / 0x4000)
            - SOUND_MAX_PITCH_CHANGE;
    }

    if (s->number < 0) {
        return;
    }

    const SOUND_MODE mode = s->flags & SOUND_MODE_MASK;
    const int32_t num_samples = (s->flags >> 2) & 0xF;
    const int32_t track_id = num_samples == 1
        ? s->number
        : s->number + (int32_t)((num_samples * Random_GetDraw()) / 0x8000);

    switch (mode) {
    case SOUND_MODE_NORMAL:
        break;

    case SOUND_MODE_WAIT:
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->sample_num == sample_num) {
                if (Audio_Sample_IsPlaying(i)) {
                    return;
                }
                M_ClearSlot(slot);
            }
        }
        break;

    case SOUND_MODE_RESTART:
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->sample_num == sample_num) {
                M_CloseSlot(slot);
                break;
            }
        }
        break;

    case SOUND_MODE_LOOPED:
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->sample_num == sample_num) {
                if (volume > slot->volume) {
                    slot->volume = volume;
                    slot->pan = pan;
                    slot->pitch = pitch;
                }
                return;
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
            if (slot->sample_num >= 0 && slot->volume < min_volume) {
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
        s->number = -1;
    } else {
        int32_t free_slot = -1;
        for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
            SOUND_SLOT *const slot = &m_SoundSlots[i];
            if (slot->sample_num < 0) {
                free_slot = i;
                break;
            }
        }

        if (free_slot != -1) {
            SOUND_SLOT *const slot = &m_SoundSlots[free_slot];
            slot->volume = volume;
            slot->pan = pan;
            slot->pitch = pitch;
            slot->sample_num = sample_num;
            slot->handle = handle;
        }
    }
}

void __cdecl Sound_StopEffect(const SOUND_EFFECT_ID sample_id)
{
    if (!g_SoundIsActive) {
        return;
    }

    const int32_t sample_num = g_SampleLUT[sample_id];
    const int32_t num_samples = (g_SampleInfos[sample_num].flags >> 2) & 0xF;

    for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
        SOUND_SLOT *const slot = &m_SoundSlots[i];
        if (slot->sample_num >= sample_num
            && slot->sample_num < sample_num + num_samples) {
            M_CloseSlot(slot);
        }
    }
}

void __cdecl Sound_StopAllSamples(void)
{
    Audio_Sample_CloseAll();
    M_ClearAllSlots();
}

void __cdecl Sound_EndScene(void)
{
    if (!g_SoundIsActive) {
        return;
    }

    for (int32_t i = 0; i < SOUND_MAX_SLOTS; i++) {
        SOUND_SLOT *const slot = &m_SoundSlots[i];
        SAMPLE_INFO *const s = &g_SampleInfos[slot->sample_num];
        if (slot->sample_num < 0) {
            continue;
        }

        if ((s->flags & SOUND_MODE_MASK) == SOUND_MODE_LOOPED) {
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

bool Sound_IsAvailable(const SOUND_EFFECT_ID sample_id)
{
    return sample_id >= 0 && sample_id < SFX_NUMBER_OF
        && g_SampleLUT[sample_id] != -1;
}
