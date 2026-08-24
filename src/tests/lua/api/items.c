// The item surface. The assertions live in items.lua; this
// stands up the world they run against.

#include <fakes/items.h>
#include <harness/lua_surface.h>

#include <trx/game/items/actions.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>

static void M_SetUpExtra(lua_State *const L)
{
    // items.lua's `room` extension hands off to trx.rooms, which is not under
    // test here.
    (void)luaL_dostring(
        L,
        "trx.rooms = setmetatable({}, {\n"
        "  __index = function(_, n) return { num = n } end })\n");
}

// fake.fire_trigger(item_num, type, mask, timer, one_shot) - mirrors the
// Item_Trigger fire site, argument for argument.
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

// fake.fire_visibility(item_num, visible) - mirrors the Item_SetVisible fire
// site, which picks the event by the value it settled on.
static int M_L_FireVisibility(lua_State *const L)
{
    LUA_FireEventInt32(
        lua_toboolean(L, 2) ? LUA_EVENT_SHOW : LUA_EVENT_HIDE,
        (int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.fire_finish(item_num) - mirrors the Item_SetFinished fire site, which
// fires only as the item becomes finished.
static int M_L_FireFinish(lua_State *const L)
{
    LUA_FireEventInt32(LUA_EVENT_FINISH, (int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.fire_sim(item_num, simulated) - mirrors the Item_AddSimulated and
// Item_RemoveSimulated fire sites.
static int M_L_FireSim(lua_State *const L)
{
    LUA_FireEventInt32(
        lua_toboolean(L, 2) ? LUA_EVENT_ENTER_SIM : LUA_EVENT_LEAVE_SIM,
        (int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.fire_activation(item_num, activated) - mirrors the Item_Activate and
// Item_Deactivate fire sites.
static int M_L_FireActivation(lua_State *const L)
{
    LUA_FireEventInt32(
        lua_toboolean(L, 2) ? LUA_EVENT_ACTIVATE : LUA_EVENT_DEACTIVATE,
        (int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.fire_destroy(item_num) - mirrors the Item_Destroy fire site.
static int M_L_FireDestroy(lua_State *const L)
{
    LUA_FireEventInt32(LUA_EVENT_DESTROY, (int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.fire_world(item_num, entered) - mirrors the Item_Initialise and
// Item_DetachFromRoom fire sites.
static int M_L_FireWorld(lua_State *const L)
{
    LUA_FireEventInt32(
        lua_toboolean(L, 2) ? LUA_EVENT_ENTER_WORLD : LUA_EVENT_LEAVE_WORLD,
        (int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// fake.fire_hit(item_num, damage) - mirrors the Item_TakeDamage fire site,
// argument for argument.
static int M_L_FireHit(lua_State *const L)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = (int32_t)luaL_checkinteger(L, 1) } },
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = (int32_t)luaL_checkinteger(L, 2) } },
    };
    LUA_FireEventEx(LUA_EVENT_HIT, args, 2);
    return 0;
}

// fake.fire_kill(item_num) - mirrors the Item_TakeDamage fire site for the
// fatal blow.
static int M_L_FireKill(lua_State *const L)
{
    LUA_FireEventInt32(LUA_EVENT_KILL, (int32_t)luaL_checkinteger(L, 1));
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
    lua_pushcfunction(L, M_L_FireVisibility);
    lua_setfield(L, -2, "fire_visibility");
    lua_pushcfunction(L, M_L_FireFinish);
    lua_setfield(L, -2, "fire_finish");
    lua_pushcfunction(L, M_L_FireSim);
    lua_setfield(L, -2, "fire_sim");
    lua_pushcfunction(L, M_L_FireActivation);
    lua_setfield(L, -2, "fire_activation");
    lua_pushcfunction(L, M_L_FireDestroy);
    lua_setfield(L, -2, "fire_destroy");
    lua_pushcfunction(L, M_L_FireWorld);
    lua_setfield(L, -2, "fire_world");
    lua_pushcfunction(L, M_L_FireHit);
    lua_setfield(L, -2, "fire_hit");
    lua_pushcfunction(L, M_L_FireKill);
    lua_setfield(L, -2, "fire_kill");
}

void ItemAction_SetInterceptor(const ITEM_ACTION_INTERCEPTOR interceptor)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "items",
        // trx.items.query is built on trx.query, and resolves an object name
        // through trx.objects.query, which matches over the catalog.
        .deps = { "strings", "catalog", "query", "objects", "events", nullptr },
        .tests = "api/items",
        .setup_extra = M_SetUpExtra,
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
