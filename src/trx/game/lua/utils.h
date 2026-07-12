#include <trx/game/objects.h>

#include <lauxlib.h>

OBJECT_PROPERTY_VALUE LUA_CheckPropertyValue(lua_State *L, int idx);
void LUA_PushPropertyValue(lua_State *L, const OBJECT_PROPERTY_VALUE *value);
