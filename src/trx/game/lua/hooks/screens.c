#include <trx/core/subsystem.h>
#include <trx/game/lua/hooks/common.h>
#include <trx/game/lua/utils/level.h>
#include <trx/game/ui/takeover.h>

typedef struct {
    UI_TAKEOVER screen;
    const UI_TAKEOVER_ARGS *args;
} M_OFFER;

static void M_SetInt(
    lua_State *const L, const char *const name, const int32_t value)
{
    lua_pushinteger(L, value);
    lua_setfield(L, -2, name);
}

static void M_SetBool(
    lua_State *const L, const char *const name, const bool value)
{
    lua_pushboolean(L, value);
    lua_setfield(L, -2, name);
}

static void M_SetBackground(
    lua_State *const L, const UI_TAKEOVER_ARGS *const args)
{
    if (args->background != nullptr) {
        lua_pushstring(L, args->background);
        lua_setfield(L, -2, "background");
    }
}

static void M_SetTiming(lua_State *const L, const UI_TAKEOVER_ARGS *const args)
{
    M_SetInt(L, "display_ticks", args->display_ticks);
    M_SetInt(L, "fade_in_ticks", args->fade_in_ticks);
    M_SetInt(L, "fade_out_ticks", args->fade_out_ticks);
}

// Pushes what the screen reports as a table, keyed as its context names the
// fields, with only the fields that the screen reports.
static void M_PushArgs(lua_State *const L, const LUA_HOOK_ARG *const arg)
{
    const M_OFFER *const offer = arg->value.ptr;
    const UI_TAKEOVER_ARGS *const args = offer->args;
    lua_newtable(L);
    switch (offer->screen) {
    case UI_TAKEOVER_RING_ENTRY:
    case UI_TAKEOVER_EXAMINE:
    case UI_TAKEOVER_COMBINE:
        M_SetInt(L, "object", args->object_id);
        break;
    case UI_TAKEOVER_SAVE_LOAD:
        M_SetInt(L, "mode", args->mode);
        break;
    case UI_TAKEOVER_STATS:
        if (args->level != nullptr) {
            LUA_PushLevel(L, args->level);
            lua_setfield(L, -2, "level");
        }
        M_SetBool(L, "is_final", args->is_final);
        M_SetBool(L, "is_bare", args->is_bare);
        M_SetBool(L, "offers_save", args->offers_save);
        M_SetBackground(L, args);
        break;
    case UI_TAKEOVER_GLOBE_SELECT:
        M_SetBackground(L, args);
        break;
    case UI_TAKEOVER_LEVEL_VIEW:
        M_SetTiming(L, args);
        break;
    case UI_TAKEOVER_PICTURE:
        M_SetBackground(L, args);
        M_SetTiming(L, args);
        M_SetBool(L, "is_loading", args->is_loading);
        M_SetBool(L, "can_cross_fade_in", args->can_cross_fade_in);
        break;
    case UI_TAKEOVER_MENU:
        M_SetInt(L, "mode", args->mode);
        if (args->object_id != NO_OBJECT) {
            M_SetInt(L, "object", args->object_id);
        }
        M_SetBackground(L, args);
        M_SetBool(L, "is_live", args->is_live);
        break;
    case UI_TAKEOVER_PAUSE:
    case UI_TAKEOVER_NUMBER_OF:
        break;
    }
}

static bool M_Offer(
    const UI_TAKEOVER screen, const UI_TAKEOVER_ARGS *const args)
{
    const M_OFFER offer = { .screen = screen, .args = args };
    return LUA_Hooks_CallBool(
        LUA_HOOK_SCREEN_OPEN, 0, false, LUA_ARG_INT(screen),
        ((LUA_HOOK_ARG) { .push = M_PushArgs, .value.ptr = &offer }));
}

static void M_Release(const UI_TAKEOVER screen)
{
    LUA_Hooks_Call(LUA_HOOK_SCREEN_RELEASE, 0, 0, LUA_ARG_INT(screen));
}

static void M_Init(void)
{
    UI_Takeover_SetHooks((UI_TAKEOVER_HOOKS) {
        .offer = M_Offer,
        .release = M_Release,
    });
}

REGISTER_SUBSYSTEM(.init = M_Init)
