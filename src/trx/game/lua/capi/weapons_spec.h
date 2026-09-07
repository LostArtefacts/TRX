#pragma once

#include <trx/core/result.h>
#include <trx/game/gun/types.h>
#include <trx/game/lara/enum.h>

#include <lua.h>

// Read a weapon spec from the table at `idx` into an existing weapon
// definition. Report failure for a name nothing stands for and for a value
// that is not the kind its key calls for, and leave the weapon unchanged
// where the spec fails. A key the reader does not know is passed over. Where
// `fire` holds a function, the weapon fires through that function instead of
// a named routine. The stack is left as it was found.
RESULT LUA_Weapons_ReadSpec(lua_State *L, int idx, WEAPON_INFO *weapon);

// States what a weapon does when it is fired, from the function at `idx`. One
// weapon holds one handler, so this replaces the handler it holds now.
void LUA_Weapons_SetFireHandler(lua_State *L, LARA_GUN_TYPE gun_type, int idx);
