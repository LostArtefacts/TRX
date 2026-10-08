#pragma once

#include <trx/game/game_flow/types.h>

#include <lua.h>

// Push a level as the trx.game.Level a script knows it by.
void LUA_PushLevel(lua_State *L, const GF_LEVEL *level);
