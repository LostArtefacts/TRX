#pragma once

#include <trx/game/game_flow/common.h>
#include <trx/game/ui/dialogs/takeover.h>

// Marks the console running a command written in Lua, for the length of it.
void LUA_Console_BeginCommand(void);
void LUA_Console_EndCommand(void);

// Returns whether the console is running a command written in Lua.
bool LUA_Console_IsRunningCommand(void);

// Queues a game flow command for a script. While a script holds a screen, the
// command waits until the screen closes, so the inventory ring spins out
// first. A console command runs at once even then, as the player typed it.
static inline void LUA_OverrideCommand(const GF_COMMAND command)
{
    GF_OverrideCommand(
        command, LUA_Console_IsRunningCommand() || !UI_Takeover_IsAnyHeld());
}
