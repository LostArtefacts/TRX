#pragma once

#include <trx/game/ui/dialogs/takeover.h>

#include <lua.h>

// Reads what a test says a screen reports, from the table at `idx`, keyed as
// the screen contexts name their fields, with `level` as the level's number
// and 0 for the gym. A missing table or field reads as zero.
UI_TAKEOVER_ARGS TakeoverArgs_Read(lua_State *L, int idx);
