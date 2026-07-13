#pragma once

#include <stdint.h>

// What the surface asked the engine to do, recorded rather than performed.
// config_writes is the one that matters: it counts the times the player's
// settings file would have been rewritten.
typedef struct {
    int32_t config_writes;
} FAKE_CONFIG_CALLS;

extern FAKE_CONFIG_CALLS g_FakeConfigCalls;

void FakeConfig_Reset(void);

// The game flow can nail an option down. A script must not be able to move it.
void FakeConfig_SetEnforced(const char *key, bool enforced);
