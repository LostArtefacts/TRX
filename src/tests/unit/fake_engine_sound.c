// A level with exactly one sample in it. Anything else is unavailable.

#include "fake_engine_sound.h"

#include <trx/core/math/types.h>
#include <trx/game/sound/common.h>
#include <trx/game/sound/ids.h>
#include <trx/game/sound/types.h>

#include <stdbool.h>

FAKE_SOUND_CALLS g_FakeSoundCalls;

typedef struct {
    bool active;
    int32_t sample_id;
} FAKE_SLOT;

static FAKE_SLOT m_Slots[FAKE_SOUND_SLOT_COUNT];

bool Sound_IsAvailable_Direct(const SAMPLE_ID sample_id)
{
    return sample_id == FAKE_SAMPLE;
}

SAMPLE_ID Sound_GetMaxDirectSampleID(void)
{
    return FAKE_SAMPLE;
}

SAMPLE_INFO *Sound_GetSample(const SAMPLE_ID sample_id)
{
    static SAMPLE_INFO sample;
    if (sample_id != FAKE_SAMPLE) {
        return nullptr;
    }
    sample = (SAMPLE_INFO) {
        .volume = FAKE_SAMPLE_VOLUME,
        .range = FAKE_SAMPLE_RANGE,
        .randomness = FAKE_SAMPLE_RANDOMNESS,
        .pitch = FAKE_SAMPLE_PITCH,
    };
    return &sample;
}

int32_t Sound_GetActiveSlotCount(void)
{
    return FAKE_SOUND_SLOT_COUNT;
}

bool Sound_GetActiveSlot(const int32_t slot, SAMPLE_ID *const out_sample_id)
{
    if (slot < 0 || slot >= FAKE_SOUND_SLOT_COUNT || !m_Slots[slot].active) {
        return false;
    }
    if (out_sample_id != nullptr) {
        *out_sample_id = m_Slots[slot].sample_id;
    }
    return true;
}

void Sound_StopActiveSlot(const int32_t slot)
{
    g_FakeSoundCalls.slot_stop_count++;
    g_FakeSoundCalls.slot_stop_slot = slot;
    if (slot >= 0 && slot < FAKE_SOUND_SLOT_COUNT) {
        m_Slots[slot].active = false;
    }
}

void Sound_PauseActiveSlot(const int32_t slot)
{
    g_FakeSoundCalls.slot_pause_count++;
    g_FakeSoundCalls.slot_pause_slot = slot;
}

void Sound_UnpauseActiveSlot(const int32_t slot)
{
    g_FakeSoundCalls.slot_unpause_count++;
    g_FakeSoundCalls.slot_unpause_slot = slot;
}

bool Sound_Effect_Direct(
    const SAMPLE_ID sfx_num, const XYZ_32 *const pos, const uint32_t flags)
{
    g_FakeSoundCalls.play_count++;
    g_FakeSoundCalls.last_sample = sfx_num;
    g_FakeSoundCalls.had_pos = pos != nullptr;
    if (pos != nullptr) {
        g_FakeSoundCalls.last_x = pos->x;
        g_FakeSoundCalls.last_y = pos->y;
        g_FakeSoundCalls.last_z = pos->z;
    }
    return true;
}

void Sound_StopEffect_Direct(const SAMPLE_ID sfx_num)
{
    g_FakeSoundCalls.stop_count++;
    g_FakeSoundCalls.last_stopped_sample = sfx_num;
}

void Sound_StopAll(void)
{
    g_FakeSoundCalls.stop_all_count++;
}

void FakeSound_Reset(void)
{
    g_FakeSoundCalls = (FAKE_SOUND_CALLS) {};
    for (int32_t i = 0; i < FAKE_SOUND_SLOT_COUNT; i++) {
        m_Slots[i] = (FAKE_SLOT) {};
    }
}

void FakeSound_SetStream(const int32_t slot, const int32_t sample_id)
{
    if (slot < 0 || slot >= FAKE_SOUND_SLOT_COUNT) {
        return;
    }
    m_Slots[slot] = (FAKE_SLOT) { .active = true, .sample_id = sample_id };
}
