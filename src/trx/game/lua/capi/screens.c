#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/args.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/ui/dialogs/takeover.h>

#include <lauxlib.h>
#include <lua.h>

// trxc.screens.close(screen: trx.ui.Screen, choice: integer)
static int M_L_ScreensClose(lua_State *const L)
{
    const UI_TAKEOVER screen = (UI_TAKEOVER)LUA_CheckRange(
        L, 1, UI_TAKEOVER_NUMBER_OF, "unknown screen");
    const UI_TAKEOVER_CHOICE choice = (UI_TAKEOVER_CHOICE)LUA_CheckRange(
        L, 2, UI_TAKEOVER_CHOICE_NUMBER_OF, "unknown screen choice");
    luaL_argcheck(
        L, UI_Takeover_AcceptsChoice(screen, choice), 2,
        "the screen does not take this choice");
    UI_Takeover_Close(screen, choice);
    return 0;
}

// trxc.screens.background(screen: trx.ui.Screen): string?
static int M_L_ScreensBackground(lua_State *const L)
{
    const UI_TAKEOVER screen = (UI_TAKEOVER)LUA_CheckRange(
        L, 1, UI_TAKEOVER_NUMBER_OF, "unknown screen");
    const char *const path = UI_Takeover_GetBackground(screen);
    if (path == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, path);
    }
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "background", M_L_ScreensBackground },
    { "close", M_L_ScreensClose },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "screens", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
