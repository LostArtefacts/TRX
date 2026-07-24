// The item surface. The assertions live in src/tests/unit/lua/items.lua; this
// stands up the world they run against.

#include "fake_engine_items.h"
#include "lua_surface.h"

#include <trx/game/items/actions.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>

// Event listeners attach against a script context; a global one lives for the
// whole session, which is all these tests need.
LUA_CONTEXT LUA_GetScriptContext(void)
{
    return LUA_CONTEXT_GLOBAL;
}

void ItemAction_SetInterceptor(const ITEM_ACTION_INTERCEPTOR interceptor)
{
}

static void M_SetUpExtra(lua_State *const L)
{
    // items.lua's `room` extension hands off to trx.rooms, which is not under
    // test here.
    (void)luaL_dostring(
        L,
        "trx.rooms = setmetatable({}, {\n"
        "  __index = function(_, n) return { num = n } end })\n");
}

static int M_FakeReset(lua_State *const L)
{
    FakeItems_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeItemCalls.destroy);
    lua_setfield(L, -2, "destroy");
    lua_pushinteger(L, g_FakeItemCalls.creature_die);
    lua_setfield(L, -2, "creature_die");
    lua_pushboolean(L, g_FakeItemCalls.creature_die_explode);
    lua_setfield(L, -2, "creature_die_explode");
    lua_pushinteger(L, g_FakeItemCalls.shatter);
    lua_setfield(L, -2, "shatter");
    lua_pushinteger(L, g_FakeItemCalls.shatter_damage);
    lua_setfield(L, -2, "shatter_damage");
    lua_pushinteger(L, g_FakeItemCalls.enable_baddie_ai);
    lua_setfield(L, -2, "enable_baddie_ai");
    lua_pushboolean(L, g_FakeItemCalls.enable_baddie_ai_forced);
    lua_setfield(L, -2, "enable_baddie_ai_forced");
    lua_pushinteger(L, g_FakeItemCalls.disable_baddie_ai);
    lua_setfield(L, -2, "disable_baddie_ai");
    return 1;
}

// fake.fire_trigger(item_num, type, mask, timer, one_shot) - mirrors the
// Item_NotifyTriggered fire site, argument for argument.
static int M_L_FireTrigger(lua_State *const L)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = (int32_t)luaL_checkinteger(L, 1) } },
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = (int32_t)luaL_checkinteger(L, 2) } },
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = (int32_t)luaL_checkinteger(L, 3) } },
        { .type = LUA_EVENT_ARG_NUMBER,
          .value = { .number = luaL_checknumber(L, 4) } },
        { .type = LUA_EVENT_ARG_BOOL, .value = { .b = lua_toboolean(L, 5) } },
    };
    LUA_FireEventEx(LUA_EVENT_TRIGGER, args, 5);
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_OBJ_WOLF);
    lua_setfield(L, -2, "WOLF");
    lua_pushinteger(L, FAKE_OBJ_VASE);
    lua_setfield(L, -2, "VASE");
    lua_pushinteger(L, FAKE_OBJ_UNLOADED);
    lua_setfield(L, -2, "UNLOADED");
    lua_pushinteger(L, FAKE_OBJ_KEY);
    lua_setfield(L, -2, "KEY");
    lua_pushcfunction(L, M_L_FireTrigger);
    lua_setfield(L, -2, "fire_trigger");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "items",
        // trx.items.query is built on trx.query, and resolves an object name
        // through trx.objects.query, which matches over the catalog.
        .deps = { "strings", "catalog", "query", "objects", "events", nullptr },
        .tests = "items",
        .setup_extra = M_SetUpExtra,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
