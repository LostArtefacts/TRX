// A level with exactly one sample in it. Anything else is unavailable.

#include <fakes/sound.h>

#include <trx/core/handle.h>
#include <trx/core/math/types.h>
#include <trx/game/sound/common.h>
#include <trx/game/sound/ids.h>
#include <trx/game/sound/types.h>

FAKE_SOUND_CALLS g_FakeSoundCalls;

typedef struct {
    bool active;
    int32_t sample_id;
} FAKE_SLOT;

static FAKE_SLOT m_Slots[FAKE_SOUND_SLOT_COUNT];
static uint32_t m_SlotGens[FAKE_SOUND_SLOT_COUNT];
static HANDLE_REGISTRY m_Handles;

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

TRX_HANDLE Sound_GetActiveSlotHandle(const int32_t slot)
{
    return Handle_RegistryMint(&m_Handles, slot);
}

bool Sound_ResolveActiveSlot(
    const TRX_HANDLE handle, SAMPLE_ID *const out_sample_id)
{
    if (!Handle_RegistryIsLive(&m_Handles, handle)) {
        return false;
    }
    return Sound_GetActiveSlot(handle.id, out_sample_id);
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

int32_t Sound_Effect_Direct(
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
    if (sfx_num != FAKE_SAMPLE) {
        return -1;
    }
    // Land the voice in a free slot and report it, as the engine does.
    for (int32_t slot = 0; slot < FAKE_SOUND_SLOT_COUNT; slot++) {
        if (!m_Slots[slot].active) {
            m_Slots[slot] =
                (FAKE_SLOT) { .active = true, .sample_id = sfx_num };
            Handle_RegistryBump(&m_Handles, slot);
            return slot;
        }
    }
    return -1;
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
    // The generations persist across the reset, as the engine's do, so a handle
    // from before it cannot match a slot that a later play reuses.
    if (m_Handles.gens == nullptr) {
        Handle_RegistryInit(&m_Handles, m_SlotGens, FAKE_SOUND_SLOT_COUNT);
    }
}

void FakeSound_SetStream(const int32_t slot, const int32_t sample_id)
{
    if (slot < 0 || slot >= FAKE_SOUND_SLOT_COUNT) {
        return;
    }
    m_Slots[slot] = (FAKE_SLOT) { .active = true, .sample_id = sample_id };
    Handle_RegistryBump(&m_Handles, slot);
}
