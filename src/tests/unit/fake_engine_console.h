#pragma once

#include <trx/core/log.h>
#include <trx/game/console/enum.h>

#include <stdbool.h>
#include <stdint.h>

// The console overlay, reduced to the last line written to it.
typedef struct {
    int32_t log_count;
    LOG_LEVEL last_level;
    char last_message[256];

    int32_t clear_count;

    int32_t eval_count;
    char last_command[256];
    // What Console_Eval saw while it ran, which is what `opts.verbose`
    // controls.
    bool verbose_during_eval;
} FAKE_CONSOLE_CALLS;

extern FAKE_CONSOLE_CALLS g_FakeConsoleCalls;

bool Console_IsVerbose(void);

void FakeConsole_Reset(void);

// The result the next Console_Eval hands back.
void FakeConsole_SetEvalResult(COMMAND_RESULT result);
