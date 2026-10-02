#pragma once

#include <trx/game/game_flow/common.h>
#include <trx/game/ui/dialogs/takeover.h>

// Queues a game flow command for a script. While a script holds a screen, the
// command waits until the screen closes, so the inventory ring spins out
// first. Otherwise, the command runs at once.
static inline void LUA_OverrideCommand(const GF_COMMAND command)
{
    GF_OverrideCommand(command, !UI_Takeover_IsAnyHeld());
}
