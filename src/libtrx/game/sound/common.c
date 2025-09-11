#include "game/sound/common.h"

#include "config.h"
#include "engine/audio.h"
#include "game/camera.h"
#include "game/game_buf.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/shell.h"
#include "game/sound.h"
#include "log.h"

#include <math.h>

typedef enum {
    SF_FLIP = 0x40,
    SF_UNFLIP = 0x80,
} SOUND_SOURCE_FLAG;

#define M_DECIBEL_LUT_SIZE 512
#if TR_VERSION == 1
    #define M_SOUND_FAR_RANGE (8 * WALL_L)
#elif TR_VERSION == 2
    #define M_SOUND_CLOSE_RANGE WALL_L
    #define M_SOUND_FAR_RANGE (10 * WALL_L)
#endif

#define M_MAX_ACTIVE_SOUNDS AUDIO_MAX_ACTIVE_SAMPLES
#define M_SOUND_RANGE_MULT_CONSTANT 4
#define M_SOUND_MAX_VOLUME                                                     \
    ((M_SOUND_FAR_RANGE * M_SOUND_RANGE_MULT_CONSTANT) - 1)
#define M_SOUND_MAX_PITCH_CHANGE 6000
#define M_SOUND_MAX_VOLUME_CHANGE 0x2000

typedef struct {
    SAMPLE_ID sample_id;
    const SAMPLE_INFO *sample;
    int32_t handle;
    int32_t volume;
    int32_t pitch;
    int32_t pan;

    const XYZ_32 *pos;
} M_ACTIVE_SOUND;

static M_ACTIVE_SOUND m_ActiveSounds[M_MAX_ACTIVE_SOUNDS] = {};

static bool m_Initialised = false;
static float m_MasterVolume = 0.0f;
static int32_t m_DecibelLUT[M_DECIBEL_LUT_SIZE] = {};

static int32_t m_SourceCount = 0;
static OBJECT_VECTOR *m_Sources = nullptr;

static int32_t m_SampleInfoCount = 0;
static int16_t m_SampleLUT[SFX_NUMBER_OF];
static SAMPLE_INFO *m_SampleInfos = nullptr;

static int32_t M_ConvertVolumeToDecibel(int32_t volume);
static int32_t M_ConvertPanToDecibel(uint16_t pan);
static float M_ConvertPitch(int32_t pitch);

static int32_t M_GetDistance(const XYZ_32 *pos);
static int32_t M_GetVolume(
    const SAMPLE_INFO *sample, int32_t distance, bool random);
static int32_t M_GetPitch(const SAMPLE_INFO *sample);
static int32_t M_GetPan(const SAMPLE_INFO *sample, const XYZ_32 *pos);

static M_ACTIVE_SOUND *M_SelectUnusedSound(void);
static M_ACTIVE_SOUND *M_SelectUsedSound(SAMPLE_ID sample_id);
static M_ACTIVE_SOUND *M_SelectUsedSoundWithPos(
    SAMPLE_ID sample_id, const XYZ_32 *pos);

static void M_ClearAllActiveSounds(void);
static void M_ClearActiveSound(M_ACTIVE_SOUND *sound);
static void M_CloseActiveSound(M_ACTIVE_SOUND *sound);
static void M_ClearActiveSoundHandles(const M_ACTIVE_SOUND *sound);

static bool M_Play(
    M_ACTIVE_SOUND *sound, const SAMPLE_INFO *sample, int32_t sample_id,
    int32_t track_id, int32_t volume, int32_t pitch, int32_t pan,
    const XYZ_32 *pos);

static void M_SyncActiveSoundHandle(M_ACTIVE_SOUND *sound);
static void M_UpdateActiveSoundParams(M_ACTIVE_SOUND *sound);

static int32_t M_ConvertVolumeToDecibel(int32_t volume)
{
    int32_t idx = volume * (m_MasterVolume / 64.0f) * M_DECIBEL_LUT_SIZE
        / M_SOUND_MAX_VOLUME;
    CLAMP(idx, 0, M_DECIBEL_LUT_SIZE - 1);
    return m_DecibelLUT[idx];
}

static int32_t M_ConvertPanToDecibel(const uint16_t pan)
{
    const int32_t result =
        sin((pan / 32767.0) * M_PI) * (M_DECIBEL_LUT_SIZE / 2);
    if (result > 0) {
        return -m_DecibelLUT[M_DECIBEL_LUT_SIZE - result];
    } else if (result < 0) {
        return m_DecibelLUT[M_DECIBEL_LUT_SIZE + result];
    } else {
        return 0;
    }
}

static float M_ConvertPitch(const int32_t pitch)
{
    return pitch / 0x10000.p0;
}

static int32_t M_GetDistance(const XYZ_32 *const pos)
{
    if (pos == nullptr) {
        return 0;
    }
#if TR_VERSION == 2
    const int32_t dx = pos->x - g_Camera.mic_pos.x;
    const int32_t dy = pos->y - g_Camera.mic_pos.y;
    const int32_t dz = pos->z - g_Camera.mic_pos.z;
#else
    const int32_t dx = pos->x - g_Camera.target.x;
    const int32_t dy = pos->y - g_Camera.target.y;
    const int32_t dz = pos->z - g_Camera.target.z;
#endif
    if (ABS(dx) > M_SOUND_FAR_RANGE || ABS(dy) > M_SOUND_FAR_RANGE
        || ABS(dz) > M_SOUND_FAR_RANGE) {
        return INT32_MAX;
    }
    uint32_t distance = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
#if TR_VERSION == 1
    return Math_Sqrt(distance);
#elif TR_VERSION == 2
    if (distance > SQUARE(M_SOUND_FAR_RANGE)) {
        return INT32_MAX;
    } else if (distance < SQUARE(M_SOUND_CLOSE_RANGE)) {
        distance = 0;
    } else {
        distance = Math_Sqrt(distance) - M_SOUND_CLOSE_RANGE;
    }
    return distance;
#endif
}

static int32_t M_GetVolume(
    const SAMPLE_INFO *const sample, const int32_t distance, const bool random)
{
#if TR_VERSION == 1
    int32_t volume = sample->volume - distance * M_SOUND_RANGE_MULT_CONSTANT;
    if (random && sample->flags.randomize_volume) {
        volume -= Random_GetDraw() * M_SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    return volume;
#elif TR_VERSION == 2
    int32_t volume = sample->volume;
    if (random && sample->flags.randomize_volume) {
        volume -= Random_GetDraw() * M_SOUND_MAX_VOLUME_CHANGE >> 15;
    }
    const int32_t attenuation =
        SQUARE(distance) / (SQUARE(M_SOUND_FAR_RANGE) / 0x10000);
    return (volume * (0x10000 - attenuation)) / 0x10000;
#endif
}

static int32_t M_GetPitch(const SAMPLE_INFO *const sample)
{
    int32_t pitch = SOUND_DEFAULT_PITCH;
#if TR_VERSION == 1
    if (!g_Config.audio.enable_pitched_sounds) {
        return pitch;
    }
#endif
    if (sample->flags.randomize_pitch) {
        pitch += ((Random_GetDraw() * M_SOUND_MAX_PITCH_CHANGE) / 0x4000)
            - M_SOUND_MAX_PITCH_CHANGE;
    }
    return pitch;
}

static int32_t M_GetPan(
    const SAMPLE_INFO *const sample, const XYZ_32 *const pos)
{
    if (pos == nullptr) {
        return 0;
    }
    const int32_t distance = M_GetDistance(pos);
#if TR_VERSION == 1
    if (distance > 0 && !sample->flags.no_pan) {
        const LARA_INFO *const lara = Lara_GetLaraInfo();
        const ITEM *const lara_item = Lara_GetItem();
        int16_t angle =
            Math_Atan(pos->z - lara_item->pos.z, pos->x - lara_item->pos.x);
        angle -= lara_item->rot.y + lara->torso_rot.y + lara->head_rot.y;
        return angle;
    }
#elif TR_VERSION == 2
    if (distance > 0 && !sample->flags.no_pan) {
        const int32_t dx = pos->x - g_Camera.mic_pos.x;
        const int32_t dz = pos->z - g_Camera.mic_pos.z;
        return (int16_t)Math_Atan(dz, dx) - g_Camera.actual_angle;
    }
#endif
    return 0;
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
    M_ACTIVE_SOUND *const sound, const SAMPLE_INFO *const sample,
    const int32_t sample_id, const int32_t track_id, const int32_t volume,
    const int32_t pitch, const int32_t pan, const XYZ_32 *const pos)
{
    M_CloseActiveSound(sound);
    const int32_t handle = Audio_Sample_Play(
        track_id, M_ConvertVolumeToDecibel(volume), M_ConvertPitch(pitch),
        M_ConvertPanToDecibel(pan), sample->mode == SAMPLE_MODE_LOOPED);
    if (handle == AUDIO_NO_SOUND) {
        return false;
    }
    sound->sample = sample;
    sound->sample_id = sample_id;
    sound->handle = handle;
    sound->volume = volume;
    sound->pitch = pitch;
    sound->pan = pan;
    sound->pos = pos;
    M_ClearActiveSoundHandles(sound);
    return true;
}

static void M_SyncActiveSoundHandle(M_ACTIVE_SOUND *const sound)
{
    Audio_Sample_SetPan(sound->handle, M_ConvertPanToDecibel(sound->pan));
    Audio_Sample_SetPitch(sound->handle, M_ConvertPitch(sound->pitch));
    Audio_Sample_SetVolume(
        sound->handle, M_ConvertVolumeToDecibel(sound->volume));
}

static void M_UpdateActiveSoundParams(M_ACTIVE_SOUND *const sound)
{
    const int32_t distance = M_GetDistance(sound->pos);
    if (distance == INT32_MAX) {
        sound->volume = 0;
        return;
    }
    int32_t volume = M_GetVolume(sound->sample, distance, false);
    if (volume < 0) {
        sound->volume = 0;
        return;
    }

    sound->volume = volume;
    sound->pan = M_GetPan(sound->sample, sound->pos);
}

bool Sound_Init(void)
{
    m_MasterVolume = g_Config.audio.sound_volume / 64.0f;
    m_DecibelLUT[0] = -10000;
    for (int32_t i = 1; i < M_DECIBEL_LUT_SIZE; i++) {
        m_DecibelLUT[i] =
            (log2(1.0 / M_DECIBEL_LUT_SIZE) - log2(1.0 / i)) * 1000;
    }

    if (!Audio_Init()) {
        LOG_ERROR("Failed to initialize libtrx sound system");
        return false;
    }

    m_Initialised = true;
    M_ClearAllActiveSounds();
    return true;
}

void Sound_Shutdown(void)
{
    m_Initialised = false;
    Audio_Shutdown();
}

bool Sound_IsInitialised(void)
{
    return m_Initialised;
}

void Sound_SetMasterVolume(const float volume)
{
    m_MasterVolume = volume * 64.0f;
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

void Sound_InitialiseSampleInfos(const int32_t num_sample_infos)
{
    m_SampleInfoCount = num_sample_infos;
    m_SampleInfos = num_sample_infos == 0
        ? nullptr
        : GameBuf_Alloc(
              sizeof(SAMPLE_INFO) * num_sample_infos, GBUF_SAMPLE_INFOS);
}

int32_t Sound_GetSampleCount(void)
{
    return m_SampleInfoCount;
}

bool Sound_LoadSample(
    const int32_t sample_num, const char *const sample_data, const size_t size)
{
    if (!Sound_IsInitialised()) {
        return false;
    }
    return Audio_Sample_Load(sample_num, sample_data, size);
}

void Sound_InitialiseSources(const int32_t num_sources)
{
    m_SourceCount = num_sources;
    m_Sources = num_sources == 0
        ? nullptr
        : GameBuf_Alloc(
              num_sources * sizeof(OBJECT_VECTOR), GBUF_SOUND_SOURCES);
}

int32_t Sound_GetSourceCount(void)
{
    return m_SourceCount;
}

OBJECT_VECTOR *Sound_GetSource(const int32_t source_idx)
{
    if (m_Sources == nullptr) {
        return nullptr;
    }
    return &m_Sources[source_idx];
}

void Sound_ResetSources(void)
{
    const bool flip_status = Room_GetFlipStatus();
    for (int32_t i = 0; i < m_SourceCount; i++) {
        OBJECT_VECTOR *const source = &m_Sources[i];
        if ((flip_status && (source->flags & SF_FLIP))
            || (!flip_status && (source->flags & SF_UNFLIP))) {
            Sound_Effect(source->data, &source->pos, SPM_NORMAL);
        }
    }
}

int16_t *Sound_GetSampleLUT(void)
{
    return m_SampleLUT;
}

SAMPLE_INFO *Sound_GetSampleInfo(const SAMPLE_ID sample_id)
{
    if (sample_id == SFX_INVALID) {
        return nullptr;
    }
    const int16_t info_idx = m_SampleLUT[sample_id];
    return info_idx < 0 ? nullptr : &m_SampleInfos[info_idx];
}

SAMPLE_INFO *Sound_GetSampleInfoByIdx(const int32_t info_idx)
{
    return &m_SampleInfos[info_idx];
}

bool Sound_IsAvailable(const SAMPLE_ID sample_id)
{
    return sample_id >= 0 && sample_id < SFX_NUMBER_OF
        && m_SampleLUT[sample_id] != -1;
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

    const SAMPLE_INFO *const sample = Sound_GetSampleInfo(sample_id);
    if (sample == nullptr || sample->number < 0) {
        return false;
    }

    if (sample->randomness && Random_GetDraw() > sample->randomness) {
        return false;
    }

    const int32_t distance = M_GetDistance(pos);
    if (distance == INT32_MAX) {
        return false;
    }

    const int32_t pan = M_GetPan(sample, pos);
    const int32_t volume = M_GetVolume(sample, distance, true);
    if (volume <= 0) {
        return false;
    }

    const int32_t pitch = M_GetPitch(sample);
    const int32_t num_samples = sample->flags.num_samples;
    const int32_t track_id = num_samples == 1
        ? sample->number
        : sample->number + ((num_samples * Random_GetDraw()) / 0x8000);

    M_ACTIVE_SOUND *sound = nullptr;
    switch (sample->mode) {
    case SAMPLE_MODE_NORMAL:
        sound = M_SelectUnusedSound();
        break;

    case SAMPLE_MODE_WAIT:
        sound = TR_VERSION == 1 ? M_SelectUsedSoundWithPos(sample_id, pos)
                                : M_SelectUsedSound(sample_id);
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

    case SAMPLE_MODE_LOOPED:
        sound = M_SelectUsedSound(sample_id);
        if (sound != nullptr) {
            if (volume > sound->volume) {
                sound->volume = volume;
                sound->pan = pan;
                sound->pitch = pitch;
            }
            return true;
        }
        sound = M_SelectUnusedSound();
        break;
    }

    if (sound == nullptr) {
        return false;
    }
    return M_Play(sound, sample, sample_id, track_id, volume, pitch, pan, pos);
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

void Sound_ResetAmbient(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }
    Sound_ResetSources();
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

        if (sound->sample->mode == SAMPLE_MODE_LOOPED) {
            if (sound->volume <= 0) {
                M_CloseActiveSound(sound);
            } else {
                M_SyncActiveSoundHandle(sound);
                sound->volume = 0;
            }
        } else if (!Audio_Sample_IsPlaying(sound->handle)) {
            M_ClearActiveSound(sound);
        } else if (TR_VERSION == 1 && sound->pos != nullptr) {
            M_UpdateActiveSoundParams(sound);
            if (sound->volume <= 0) {
                M_CloseActiveSound(sound);
            } else {
                M_SyncActiveSoundHandle(sound);
            }
        }
    }
}

void Sound_PauseAll(void)
{
    Audio_Sample_PauseAll();
}

void Sound_UnpauseAll(void)
{
    Audio_Sample_UnpauseAll();
}

void Sound_StopAll(void)
{
    if (!Sound_IsInitialised()) {
        return;
    }
    Audio_Sample_CloseAll();
    M_ClearAllActiveSounds();
}
