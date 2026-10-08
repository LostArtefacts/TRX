#pragma once

#include <lua.h>
#include <stdint.h>

// Push an item as the handle trx.items hands out, so that a module other than
// that one can give a script an item to work with.
void LUA_PushItem(lua_State *L, int16_t item_num);
