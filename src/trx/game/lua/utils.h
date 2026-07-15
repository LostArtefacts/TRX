#include <trx/game/objects.h>

#include <lauxlib.h>

OBJECT_PROPERTY_VALUE LUA_CheckPropertyValue(lua_State *L, int idx);
void LUA_PushPropertyValue(lua_State *L, const OBJECT_PROPERTY_VALUE *value);

// An argument the engine indexes one of its own tables with, unchecked.
// Narrowed only once it fits, so a wider value cannot wrap into range.
int32_t LUA_CheckRange(lua_State *L, int arg, int32_t count, const char *what);

// An object id, checked against the object table. Object_Get asserts on one
// outside it.
OBJECT_ID LUA_CheckObjectID(lua_State *L, int arg);
