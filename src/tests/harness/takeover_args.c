#include <harness/takeover_args.h>

#include <trx/game/game_flow/common.h>

#include <lauxlib.h>
#include <stdio.h>

static int32_t M_IntField(
    lua_State *const L, const int idx, const char *const name,
    const int32_t fallback)
{
    lua_getfield(L, idx, name);
    const int32_t value = (int32_t)luaL_optinteger(L, -1, fallback);
    lua_pop(L, 1);
    return value;
}

static bool M_BoolField(
    lua_State *const L, const int idx, const char *const name)
{
    lua_getfield(L, idx, name);
    const bool value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}

UI_TAKEOVER_ARGS TakeoverArgs_Read(lua_State *const L, const int idx)
{
    static char background[256];
    UI_TAKEOVER_ARGS args = {};
    if (!lua_istable(L, idx)) {
        return args;
    }
    args.object_id = (OBJECT_ID)M_IntField(L, idx, "object", NO_OBJECT);
    args.key_id = (OBJECT_ID)M_IntField(L, idx, "key", NO_OBJECT);
    args.mode = (INVENTORY_MODE)M_IntField(L, idx, "mode", 0);
    lua_getfield(L, idx, "level");
    if (!lua_isnil(L, -1)) {
        const int32_t num = (int32_t)luaL_checkinteger(L, -1);
        args.level = num == 0 ? GF_GetGymLevel()
                              : GF_GetLevelByOrdinalNumber(GFLT_MAIN, num);
    }
    lua_pop(L, 1);
    args.is_final = M_BoolField(L, idx, "is_final");
    args.is_bare = M_BoolField(L, idx, "is_bare");
    lua_getfield(L, idx, "background");
    if (!lua_isnil(L, -1)) {
        snprintf(background, sizeof(background), "%s", luaL_checkstring(L, -1));
        args.background = background;
    }
    lua_pop(L, 1);
    args.display_ticks = M_IntField(L, idx, "display_ticks", 0);
    args.fade_in_ticks = M_IntField(L, idx, "fade_in_ticks", 0);
    args.fade_out_ticks = M_IntField(L, idx, "fade_out_ticks", 0);
    args.is_loading = M_BoolField(L, idx, "is_loading");
    args.can_cross_fade_in = M_BoolField(L, idx, "can_cross_fade_in");
    args.can_exit_to_title = M_BoolField(L, idx, "can_exit_to_title");
    return args;
}
