#pragma once

#include <trx/core/log.h>
#include <trx/game/console/enum.h>

#include <lualib.h>
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

// Runs a command the way the console does when the player types it.
COMMAND_RESULT FakeConsole_Run(const char *prefix, const char *args);
int32_t FakeConsole_CommandCount(void);
const char *FakeConsole_HelpId(const char *prefix);

void FakeConsole_Reset(void);

// The result the next Console_Eval hands back.
void FakeConsole_SetEvalResult(COMMAND_RESULT result);

// The console as a test script sees it: fake.run(), fake.help_id(),
// fake.is_registered(), fake.set_eval_result(), fake.as_level_script(),
// fake.reload(), and fake.CommandResult.
void FakeConsole_PushLua(lua_State *L);

// Adds what the console was asked to do to the table on top of the stack.
void FakeConsole_PushCalls(lua_State *L);
