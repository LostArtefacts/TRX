#pragma once

#include <trx/core/log.h>

#include <stdint.h>

// The log file, reduced to the last line written to it plus where it came from.
typedef struct {
    int32_t count;
    LOG_LEVEL last_level;
    char last_message[256];
    char last_func[128];
    int32_t last_line;
} FAKE_LOG_CALLS;

extern FAKE_LOG_CALLS g_FakeLogCalls;

void FakeLog_Reset(void);
