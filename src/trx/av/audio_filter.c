// Fourth-order Butterworth low-pass made of two state-variable stages in the
// topology-preserving form by Vadim Zavalishin and Andrew Simper, which stays
// stable while its cutoff moves.

#include <trx/av/audio_internal.h>
#include <trx/core/utils.h>

#include <math.h>

#define M_OPEN_CUTOFF 20000.0f
#define M_GLIDE_FRAMES 32
#define M_GLIDE_TIME 0.08f
#define M_SNAP_DISTANCE 0.001f
#define M_DENORMAL_EPSILON 1e-15f

// Damping of each stage, 2 cos(pi / 8) and 2 cos(3 pi / 8), which together
// give the maximally flat Butterworth response.
static const float m_StageDamping[AUDIO_FILTER_STAGES] = {
    1.847759f,
    0.765367f,
};

static float M_Undenormalize(const float value)
{
    return fabsf(value) < M_DENORMAL_EPSILON ? 0.0f : value;
}

static float M_GetGlideRate(void)
{
    return 1.0f
        - expf(-(float)M_GLIDE_FRAMES / (M_GLIDE_TIME * AUDIO_WORKING_RATE));
}

static void M_Prime(AUDIO_FILTER *const filter, const float *const frame)
{
    for (int32_t stage = 0; stage < AUDIO_FILTER_STAGES; stage++) {
        for (int32_t channel = 0; channel < AUDIO_WORKING_CHANNELS; channel++) {
            filter->band[stage][channel] = 0.0f;
            filter->low[stage][channel] = frame[channel];
        }
    }
}

static void M_Glide(AUDIO_FILTER *const filter)
{
    const float distance = filter->target_log - filter->cutoff_log;
    if (fabsf(distance) < M_SNAP_DISTANCE) {
        filter->cutoff_log = filter->target_log;
    } else {
        filter->cutoff_log += distance * M_GetGlideRate();
    }
    filter->gain = tanf(
        (float)M_PI * expf(filter->cutoff_log) / (float)AUDIO_WORKING_RATE);
}

static bool M_IsOpen(const AUDIO_FILTER *const filter)
{
    const float open_log = logf(M_OPEN_CUTOFF);
    return filter->cutoff_log >= open_log && filter->target_log >= open_log;
}

static void M_ProcessFrames(
    AUDIO_FILTER *const filter, float *const buffer, const uint32_t frames)
{
    const float g = filter->gain;
    for (int32_t stage = 0; stage < AUDIO_FILTER_STAGES; stage++) {
        const float k = m_StageDamping[stage];
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        for (int32_t channel = 0; channel < AUDIO_WORKING_CHANNELS; channel++) {
            float ic1 = filter->band[stage][channel];
            float ic2 = filter->low[stage][channel];
            float *sample = buffer + channel;
            for (uint32_t i = 0; i < frames; i++) {
                const float v3 = *sample - ic2;
                const float v1 = a1 * ic1 + a2 * v3;
                const float v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1;
                ic2 = 2.0f * v2 - ic2;
                *sample = v2;
                sample += AUDIO_WORKING_CHANNELS;
            }
            filter->band[stage][channel] = M_Undenormalize(ic1);
            filter->low[stage][channel] = M_Undenormalize(ic2);
        }
    }
}

void Audio_Filter_Reset(AUDIO_FILTER *const filter)
{
    *filter = (AUDIO_FILTER) {
        .cutoff_log = logf(M_OPEN_CUTOFF),
        .target_log = logf(M_OPEN_CUTOFF),
    };
}

void Audio_Filter_SetCutoff(AUDIO_FILTER *const filter, const float cutoff)
{
    const float target =
        cutoff > 0.0f && cutoff < M_OPEN_CUTOFF ? cutoff : M_OPEN_CUTOFF;
    filter->target_log = logf(target);
}

void Audio_Filter_Process(
    AUDIO_FILTER *const filter, float *buffer, uint32_t count)
{
    uint32_t frames = count / AUDIO_WORKING_CHANNELS;
    while (frames > 0) {
        if (M_IsOpen(filter)) {
            filter->is_active = false;
            return;
        }
        if (!filter->is_active) {
            M_Prime(filter, buffer);
            filter->is_active = true;
        }
        M_Glide(filter);
        const uint32_t block = MIN(frames, (uint32_t)M_GLIDE_FRAMES);
        M_ProcessFrames(filter, buffer, block);
        buffer += block * AUDIO_WORKING_CHANNELS;
        frames -= block;
    }
}
