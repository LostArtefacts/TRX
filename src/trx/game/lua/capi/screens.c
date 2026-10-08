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

static const luaL_Reg m_Module[] = {
    { "close", M_L_ScreensClose },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "screens", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
