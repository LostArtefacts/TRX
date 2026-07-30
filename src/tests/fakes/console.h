#pragma once

#include <trx/core/log.h>
#include <trx/game/console/enum.h>

#include <lualib.h>
#include <stdint.h>

bool Console_IsVerbose(void);

// Runs a command the way the console does when the player types it.
COMMAND_RESULT FakeConsole_Run(const char *prefix, const char *args);
int32_t FakeConsole_CommandCount(void);
const char *FakeConsole_HelpId(const char *prefix);

// The result the next Console_Eval hands back.
void FakeConsole_SetEvalResult(COMMAND_RESULT result);

// The console as a test script sees it: fake.run(), fake.help_id(),
// fake.is_registered(), fake.set_eval_result(), fake.as_level_script(),
// fake.reload(), fake.is_verbose(), and fake.CommandResult.
void FakeConsole_PushLua(lua_State *L);
