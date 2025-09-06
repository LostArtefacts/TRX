#include "game/sound/common.h"

#include "config.h"
#include "engine/audio.h"
#include "game/game_buf.h"
#include "game/rooms.h"
#include "log.h"
#include "utils.h"

typedef enum {
    SF_FLIP = 0x40,
    SF_UNFLIP = 0x80,
} SOUND_SOURCE_FLAG;

#define M_DECIBEL_LUT_SIZE 512
#if TR_VERSION == 1
    #define M_SOUND_RANGE 8
#elif TR_VERSION == 2
    #define M_SOUND_RANGE 10
#endif
#define M_SOUND_RANGE_MULT_CONSTANT 4
#define M_SOUND_RADIUS (M_SOUND_RANGE * WALL_L)
#define M_SOUND_MAX_VOLUME ((M_SOUND_RADIUS * M_SOUND_RANGE_MULT_CONSTANT) - 1)

static bool m_Initialised = false;
static float m_MasterVolume = 0.0f;
static int32_t m_DecibelLUT[M_DECIBEL_LUT_SIZE] = {};

static int32_t m_SourceCount = 0;
static OBJECT_VECTOR *m_Sources = nullptr;

static int32_t m_SampleInfoCount = 0;
static int16_t m_SampleLUT[SFX_NUMBER_OF];
static SAMPLE_INFO *m_SampleInfos = nullptr;

// TODO: make static
int32_t Sound_ConvertVolumeToDecibel(int32_t volume)
{
    int32_t idx = volume * (m_MasterVolume / 64.0f) * M_DECIBEL_LUT_SIZE
        / M_SOUND_MAX_VOLUME;
    CLAMP(idx, 0, M_DECIBEL_LUT_SIZE - 1);
    return m_DecibelLUT[idx];
}

// TODO: make static
int32_t Sound_ConvertPanToDecibel(const uint16_t pan)
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

// TODO: inline
bool Sound_InitialiseCommon(void)
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

void Sound_InitialiseSources(const int32_t num_sources)
{
    m_SourceCount = num_sources;
    m_Sources = num_sources == 0
        ? nullptr
        : GameBuf_Alloc(
              num_sources * sizeof(OBJECT_VECTOR), GBUF_SOUND_SOURCES);
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

void Sound_PauseAll(void)
{
    Audio_Sample_PauseAll();
}

void Sound_UnpauseAll(void)
{
    Audio_Sample_UnpauseAll();
}

bool Sound_IsAvailable(const SAMPLE_ID sample_id)
{
    return sample_id >= 0 && sample_id < SFX_NUMBER_OF
        && m_SampleLUT[sample_id] != -1;
}
