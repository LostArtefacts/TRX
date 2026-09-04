#include <harness/harness.h>

#include <harness/stubs_game_script.h>
#include <trx/core/result.h>
#include <trx/core/strings/common.h>
#include <trx/core/utils.h>
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
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
    #include <direct.h>
#else
    #include <unistd.h>
#endif

// Where the resolver stub is pointed at the scripts a test writes. They land
// under the working directory and are taken away again, so a run by hand from
// the source tree leaves nothing behind.
#define M_GAMES_DIR "lua_scripts_games"
#define M_COMMON_DIR "lua_scripts_common"
#define M_SCRIPTS_DIR "lua_scripts_engine"

typedef struct {
    char path[256];
    bool is_dir;
} M_WRITTEN;

static M_WRITTEN m_Written[32];
static int32_t m_WrittenCount = 0;

// A level with no script file of its own still counts as a run: the context is
// the level's from here, and what attaches meanwhile is the level's to drop.
static const GF_LEVEL m_Level = { .script_path = nullptr };

static int M_L_Nothing(lua_State *const L)
{
    return 0;
}

static char *M_Remember(const char *const path, const bool is_dir)
{
    CHECK(m_WrittenCount < (int32_t)ARRAY_SIZE(m_Written));
    M_WRITTEN *const written = &m_Written[m_WrittenCount++];
    snprintf(written->path, sizeof(written->path), "%s", path);
    written->is_dir = is_dir;
    return written->path;
}

static void M_MakeDir(const char *const path)
{
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
    M_Remember(path, true);
}

// Newest first, so a directory goes only once what is in it has.
static void M_ClearScripts(void)
{
    while (m_WrittenCount > 0) {
        const M_WRITTEN *const written = &m_Written[--m_WrittenCount];
        if (written->is_dir) {
#ifdef _WIN32
            _rmdir(written->path);
#else
            rmdir(written->path);
#endif
        } else {
            remove(written->path);
        }
    }
    FakeGameScript_SetScriptDir(GAME_DYNAMIC_PATH_GAME_MODULE_FILE, nullptr);
    FakeGameScript_SetScriptDir(GAME_DYNAMIC_PATH_COMMON_MODULE_FILE, nullptr);
    FakeGameScript_SetScriptDir(GAME_DYNAMIC_PATH_GAME_SCRIPT_FILE, nullptr);
}

// A name carries directories of its own, so each of them is made in turn
// before the file lands.
static char *M_WriteScriptIn(
    const GAME_DYNAMIC_PATH source, const char *const dir,
    const char *const name, const char *const body)
{
    M_MakeDir(dir);

    char path[256];
    snprintf(path, sizeof(path), "%s/%s.lua", dir, name);
    for (char *ch = strchr(path + strlen(dir) + 1, '/'); ch != nullptr;
         ch = strchr(ch + 1, '/')) {
        *ch = '\0';
        M_MakeDir(path);
        *ch = '/';
    }

    FILE *const fp = fopen(path, "wb");
    CHECK_NOT_NULL(fp);
    fputs(body, fp);
    fclose(fp);

    char *const kept = M_Remember(path, false);
    FakeGameScript_SetScriptDir(source, dir);
    return kept;
}

static void M_WriteGameScript(const char *const name, const char *const body)
{
    char rel[192];
    snprintf(rel, sizeof(rel), "tr1/modules/%s", name);
    M_WriteScriptIn(GAME_DYNAMIC_PATH_GAME_MODULE_FILE, M_GAMES_DIR, rel, body);
}

static void M_WriteCommonScript(const char *const name, const char *const body)
{
    M_WriteScriptIn(
        GAME_DYNAMIC_PATH_COMMON_MODULE_FILE, M_COMMON_DIR, name, body);
}

// What the engine runs rather than a script requires: _game.lua, and the level
// scripts a level carries the path of.
static char *M_WriteEngineScript(const char *const name, const char *const body)
{
    return M_WriteScriptIn(
        GAME_DYNAMIC_PATH_GAME_SCRIPT_FILE, M_SCRIPTS_DIR, name, body);
}

static void M_Booted(void)
{
    M_ClearScripts();
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    EXIT_ON_FAIL(LUA_Init(), "failed to start Lua");
}

static void M_Done(void)
{
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    LUA_Shutdown();
    M_ClearScripts();
}

static void M_CheckEval(const char *const code)
{
    LUA_RESULT res = LUA_Eval(code);
    if (res.code != LUA_OK) {
        TEST_FAIL("%s", res.message);
    }
    LUA_FreeResult(&res);
}

static void M_CheckEvalFails(const char *const code, const char *const needle)
{
    LUA_RESULT res = LUA_Eval(code);
    if (res.code == LUA_OK) {
        TEST_FAIL("%s: expected an error", code);
    }
    if (strstr(res.message, needle) == nullptr) {
        TEST_FAIL("expected '%s', got '%s'", needle, res.message);
    }
    LUA_FreeResult(&res);
}

// A bare state carrying the real event registry, listening for the unload a
// global script would have set up a listener for.
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

const LUA_EMBEDDED_SCRIPT g_LUA_EmbeddedModules[] = { { nullptr } };
const LUA_EMBEDDED_SCRIPT g_LUA_EmbeddedRuntimeScripts[] = { { nullptr } };

const GF_LEVEL *GF_GetCurrentLevel(void)
{
    return nullptr;
}

void ItemAction_SetInterceptor(ITEM_ACTION_INTERCEPTOR interceptor)
{
}

// The API registry that would answer is not linked here, and a boot that gets
// no sealer exits the game.
bool LUA_API_PushEntrypoint(lua_State *const L, const char *const name)
{
    lua_pushcfunction(L, M_L_Nothing);
    return true;
}

// The config bridge that keeps the watchers is not linked here; a script
// lifecycle is what these tests are about, and no config exists to watch.
void LUA_Config_ClearLevelWatchers(void)
{
}

// Keep flip-group declarations outside the unlinked room bridge.
void LUA_Rooms_ClearFlipGroups(void)
{
}

void LUA_Guard_Install(lua_State *const L, const double budget_sec)
{
}

void LUA_HardenGlobals(lua_State *const L)
{
}

void LUA_OpenSafeLibs(lua_State *const L)
{
    luaL_openlibs(L);
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

// The stats scan runs every level's script to count what the level holds. Such
// a run leaves the same state behind as a level being played, so it is let go
// of the same way.
TEST(a_probe_run_is_let_go_of_as_a_level_is)
{
    lua_State *const L = M_Listening();

    LUA_RunLevelScript(&m_Level);
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 1);

    // The scan leaves nothing outstanding, so the level that opens the game
    // finds nothing to let go of.
    LUA_DropLevelScript();
    CHECK_EQ_INT(M_Heard(L), 1);

    LUA_Shutdown();
    lua_close(L);
}

TEST(a_required_script_runs_once_and_hands_back_what_it_returned)
{
    M_Booted();
    M_WriteGameScript(
        "counted", "runs = (runs or 0) + 1\nreturn { n = runs }\n");

    M_CheckEval(
        "local a = require('tr1.counted')\n"
        "local b = require('tr1.counted')\n"
        "assert(a == b and a.n == 1 and runs == 1)\n");

    M_Done();
}

// A module is free to be worth false. Marking a run in progress with the same
// value would have the second require of one told it requires itself.
TEST(a_required_script_may_return_false)
{
    M_Booted();
    M_WriteGameScript("falsy", "return false\n");

    M_CheckEval(
        "assert(require('tr1.falsy') == false)\n"
        "assert(require('tr1.falsy') == false)\n");

    M_Done();
}

TEST(a_script_requiring_itself_is_told_so)
{
    M_Booted();
    M_WriteGameScript("looped", "return require('tr1.looped')\n");

    M_CheckEvalFails("require('tr1.looped')", "requires itself");

    M_Done();
}

TEST(a_required_script_may_sit_in_a_subdirectory)
{
    M_Booted();
    M_WriteGameScript("my_group/my_module", "return { name = 'nested' }\n");

    M_CheckEval("assert(require('tr1.my_group.my_module').name == 'nested')");

    M_Done();
}

// A directory with an init.lua answers to the name of the directory, so a
// module that outgrows one file keeps the name its callers write.
TEST(a_directory_with_an_init_script_answers_to_its_name)
{
    M_Booted();
    M_WriteGameScript("my_group/init", "return { name = 'grouped' }\n");
    M_WriteCommonScript("their_group/init", "return { name = 'pooled' }\n");

    M_CheckEval("assert(require('tr1.my_group').name == 'grouped')");
    M_CheckEval("assert(require('common.their_group').name == 'pooled')");

    M_Done();
}

// The file wins, so a directory beside a script of the same name cannot take
// the name over.
TEST(a_script_answers_before_the_directory_beside_it)
{
    M_Booted();
    M_WriteGameScript("my_group", "return { name = 'file' }\n");
    M_WriteGameScript("my_group/init", "return { name = 'directory' }\n");

    M_CheckEval("assert(require('tr1.my_group').name == 'file')");

    M_Done();
}

// The name is the whole of what keeps a require inside the directory it names,
// there being no other check between it and the filesystem.
TEST(a_name_that_could_reach_outside_is_refused)
{
    M_Booted();

    M_CheckEval(
        "for _, name in ipairs({'../secret', 'tr1/my_module', '/etc/passwd',\n"
        "    '..', 'tr1..a', '.my_module', 'tr1.', '', 'tr1\\\\a'}) do\n"
        "  local ok, err = pcall(require, name)\n"
        "  assert(not ok, 'require accepted ' .. name)\n"
        "  assert(err:find('not a script name', 1, true), name)\n"
        "end\n");

    M_Done();
}

// A bare name says nothing about which directory it means, and there is no
// directory a require falls back to.
TEST(a_name_carrying_no_directory_is_refused)
{
    M_Booted();
    M_WriteGameScript("my_module", "return 1\n");

    M_CheckEvalFails(
        "require('my_module')", "carries the directory it lives in");

    M_Done();
}

TEST(the_engines_own_root_is_not_required)
{
    M_Booted();

    M_CheckEvalFails("require('trx.items')", "reached as a global");

    M_Done();
}

TEST(a_script_missing_from_the_directory_is_said_to_be)
{
    M_Booted();

    M_CheckEvalFails("require('tr1.absent')", "no such script");

    M_Done();
}

TEST(a_game_and_the_pool_hold_separate_modules)
{
    M_Booted();
    M_WriteGameScript("my_module", "return 'game'\n");
    M_WriteCommonScript("my_module", "return 'common'\n");

    M_CheckEval(
        "assert(require('tr1.my_module') == 'game')\n"
        "assert(require('common.my_module') == 'common')\n");

    M_Done();
}

// Path resolution ignores case, so two spellings reach one file.
TEST(a_name_spelled_in_another_case_is_the_same_module)
{
    M_Booted();
    M_WriteGameScript("counted", "runs = (runs or 0) + 1\nreturn runs\n");

    M_CheckEval(
        "assert(require('tr1.counted') == 1)\n"
        "assert(require('TR1.Counted') == 1)\n"
        "assert(runs == 1)\n");

    M_Done();
}

// The require is written in a module of its own, so the spelling under test is
// nowhere in the chunk name Lua puts in front of the message.
TEST(a_name_that_reaches_nothing_is_reported_as_it_was_written)
{
    M_Booted();
    M_WriteGameScript("caller", "return require('TR1.Absent')\n");

    M_CheckEvalFails("require('tr1.caller')", "no such script: TR1.Absent");

    M_Done();
}

// scripts/ is what the engine runs, and a name reaches modules/ alone.
TEST(a_script_the_engine_runs_is_out_of_reach_of_a_name)
{
    M_Booted();
    M_WriteScriptIn(
        GAME_DYNAMIC_PATH_GAME_MODULE_FILE, M_GAMES_DIR, "tr1/scripts/gym",
        "return 'a level script'\n");

    M_CheckEvalFails("require('tr1.gym')", "no such script");

    M_Done();
}

TEST(a_module_a_level_required_runs_again_for_the_next_level)
{
    M_Booted();
    M_WriteGameScript("counted", "runs = (runs or 0) + 1\nreturn runs\n");

    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    M_CheckEval("assert(require('tr1.counted') == 1)");
    M_CheckEval("assert(require('tr1.counted') == 1)");
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);

    LUA_DropLevelScript();
    M_CheckEval("assert(require('tr1.counted') == 2)");

    M_Done();
}

TEST(a_module_the_game_script_required_stays_across_levels)
{
    M_Booted();
    M_WriteGameScript("counted", "runs = (runs or 0) + 1\nreturn runs\n");

    M_CheckEval("assert(require('tr1.counted') == 1)");

    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    M_CheckEval("assert(require('tr1.counted') == 1)");
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);

    LUA_DropLevelScript();
    M_CheckEval("assert(require('tr1.counted') == 1)");

    M_Done();
}

// The console runs at the game's context while a level is loaded, and a
// require typed there takes neither a run of its own nor the name for the run.
TEST(a_module_a_level_loaded_is_handed_over_rather_than_run_again)
{
    M_Booted();
    M_WriteGameScript("counted", "runs = (runs or 0) + 1\nreturn runs\n");

    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    M_CheckEval("assert(require('tr1.counted') == 1)");
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    M_CheckEval("assert(require('tr1.counted') == 1)");

    LUA_DropLevelScript();
    M_CheckEval("assert(require('tr1.counted') == 2)");

    M_Done();
}

TEST(the_game_script_runs_as_the_game_starts)
{
    M_Booted();
    M_WriteEngineScript("_game", "game_runs = (game_runs or 0) + 1\n");

    LUA_RunGameScript();
    M_CheckEval("assert(game_runs == 1)");

    M_Done();
}

// The directory holds a level script, so the miss is _game.lua being absent
// rather than nowhere to look.
TEST(a_game_shipping_no_script_of_its_own_runs_none)
{
    M_Booted();
    M_WriteEngineScript("gym", "game_runs = 1\n");

    LUA_RunGameScript();
    M_CheckEval("assert(game_runs == nil)");

    M_Done();
}

TEST(a_level_script_runs_from_the_path_the_level_carries)
{
    M_Booted();
    const GF_LEVEL level = { .script_path = M_WriteEngineScript(
                                 "gym",
                                 "level_runs = (level_runs or 0) + 1\n"
                                 "require('tr1.counted')\n") };
    M_WriteGameScript("counted", "runs = (runs or 0) + 1\nreturn runs\n");

    LUA_RunLevelScript(&level);
    M_CheckEval("assert(level_runs == 1 and runs == 1)");

    LUA_DropLevelScript();
    LUA_RunLevelScript(&level);
    M_CheckEval("assert(level_runs == 2 and runs == 2)");

    M_Done();
}

TEST(what_the_game_script_required_outlives_a_level)
{
    M_Booted();
    M_WriteGameScript("counted", "runs = (runs or 0) + 1\nreturn runs\n");
    M_WriteEngineScript("_game", "require('tr1.counted')\n");
    const GF_LEVEL level = { .script_path = M_WriteEngineScript(
                                 "gym", "require('tr1.counted')\n") };

    LUA_RunGameScript();
    M_CheckEval("assert(runs == 1)");

    LUA_RunLevelScript(&level);
    LUA_DropLevelScript();
    M_CheckEval("assert(runs == 1)");

    M_Done();
}
