// A level with exactly one sample in it. Anything else is unavailable.

#include "fake_engine_sound.h"

#include <trx/core/math/types.h>
#include <trx/game/sound/ids.h>

#include <stdbool.h>

FAKE_SOUND_CALLS g_FakeSoundCalls;

bool Sound_IsAvailable_Direct(const SAMPLE_ID sample_id)
{
    return sample_id == FAKE_SAMPLE;
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
}
