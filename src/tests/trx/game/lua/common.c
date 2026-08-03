#include <harness/harness.h>

#include <trx/core/strings/common.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/items/actions.h>
#include <trx/game/lua/api.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/embedded_scripts.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/guard.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/sandbox.h>

#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>

// Letting go of a level's script, against a bare state carrying the real event
// registry. What a global script would have set up is a listener on the unload,
// and how often it is reached is the whole assertion.

// Boot, the API dump and the reload all reach outside common.c, and none of the
// three is walked from here. A link error rather than a stub is what should
// meet the next thing the drop starts reaching for.
const LUA_EMBEDDED_SCRIPT g_LUA_EmbeddedModules[] = { { nullptr } };
const LUA_EMBEDDED_SCRIPT g_LUA_EmbeddedRuntimeScripts[] = { { nullptr } };

const GF_LEVEL *GF_GetCurrentLevel(void)
{
    return nullptr;
}

void ItemAction_SetInterceptor(ITEM_ACTION_INTERCEPTOR interceptor)
{
}

bool LUA_API_PushEntrypoint(lua_State *const L, const char *const name)
{
    return false;
}

void LUA_Guard_Install(lua_State *const L, const double budget_sec)
{
}

void LUA_HardenGlobals(lua_State *const L)
{
}

void LUA_OpenSafeLibs(lua_State *const L)
{
}

void Shell_ExitSystem(const char *const message)
{
    TEST_FAIL("Shell_ExitSystem: %s", message);
}

void Shell_ExitSystemFmt(const char *const fmt, ...)
{
    TEST_FAIL("Shell_ExitSystemFmt: %s", fmt);
}

char *String_Format(const char *const fmt, ...)
{
    return nullptr;
}

static lua_State *M_Listening(void)
{
    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);
    lua_newtable(L);
    lua_setglobal(L, "trxc");
    LUA_Registry_CreateAll(L);

    const char *const attach =
        "count = 0\n"
        "trxc.events.attach(%d, function()\n"
        "  count = count + 1\n"
        "end)\n";
    char src[256];
    snprintf(src, sizeof(src), attach, (int)LUA_EVENT_LEVEL_UNLOAD);
    CHECK(luaL_dostring(L, src) == LUA_OK);
    return L;
}

static int32_t M_Heard(lua_State *const L)
{
    lua_getglobal(L, "count");
    const int32_t count = (int32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return count;
}

// A level with no script file of its own still counts as a run: the context is
// the level's from here, and what attaches meanwhile is the level's to drop.
static const GF_LEVEL m_Level = { .script_path = nullptr };

TEST(the_unload_opening_a_session_says_nothing)
{
    lua_State *const L = M_Listening();

    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 0);

    LUA_Shutdown();
    lua_close(L);
}

TEST(a_level_script_run_is_let_go_of_once)
{
    lua_State *const L = M_Listening();

    LUA_RunLevelScript(&m_Level);
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 1);

    // The next level load drops again before its own script runs, and there is
    // nothing left outstanding to drop.
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 1);

    LUA_RunLevelScript(&m_Level);
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 2);

    LUA_Shutdown();
    lua_close(L);
}

// The stats scan runs every level's script to count what the level holds.
TEST(a_probe_run_is_let_go_of_without_a_word)
{
    lua_State *const L = M_Listening();

    LUA_SetLevelScriptProbing(true);
    LUA_RunLevelScript(&m_Level);
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 0);
    LUA_SetLevelScriptProbing(false);

    // The scan leaves nothing outstanding, so the level that opens the game
    // finds nothing to let go of either.
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 0);

    // And a level played after one says so as any other does.
    LUA_RunLevelScript(&m_Level);
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 1);

    LUA_Shutdown();
    lua_close(L);
}
