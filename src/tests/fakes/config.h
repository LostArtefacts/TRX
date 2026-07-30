#pragma once

#include <trx/config/option.h>

#include <stdint.h>

// What the surface asked the engine to do, recorded rather than performed.
// config_writes counts the times the player's settings file would have been
// rewritten, which is the difference between set() and override().
typedef struct {
    int32_t config_writes;
} FAKE_CONFIG_CALLS;

extern FAKE_CONFIG_CALLS g_FakeConfigCalls;

void FakeConfig_Reset(void);
void FakeConfig_SetEnforced(bool enforced);
