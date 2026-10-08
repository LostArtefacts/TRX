#include <trx/game/lua/scripts.h>

#include <trx/core/log.h>
#include <trx/game/console.h>
#include <trx/game/game_flow.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>
#include <trx/game/paths.h>

// Whether a level script has run since the last drop, so the unload that opens
// the first level of a session passes in silence.
static bool m_LevelScriptLive = false;

static void M_Shutdown(void)
{
    m_LevelScriptLive = false;
}

RESULT LUA_RunGameScript(void)
{
    // An expansion with nothing of its own to set up runs the script of the
    // game it extends, so it ships a file only to replace one. What it wants
    // to keep from the game it extends it requires by name.
    const char *const path =
        GamePath_PeekResolve(GAME_DYNAMIC_PATH_GAME_SCRIPT_FILE, "_game.lua");
    if (path == nullptr) {
        return OK;
    }

    LOG_INFO("Loading game script: %s", path);
    RESULT result = OK;
    LUA_RESULT res = LUA_EvalFile(path);
    if (res.code != LUA_OK) {
        result = FAIL("%s", res.message);
    }
    LUA_FreeResult(&res);
    return result;
}

void LUA_DropLevelScript(void)
{
    if (m_LevelScriptLive) {
        m_LevelScriptLive = false;

        // Before the listeners go, so a module holding state the level set up
        // hears about it while its own handlers still answer and can take them
        // down itself.
        LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    }

    LUA_Registry_DropLevelAll();
    LUA_DropLevelModules();
}

void LUA_RunLevelScript(const GF_LEVEL *const level)
{
    m_LevelScriptLive = true;
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);

    if (level->script_path != nullptr) {
        LUA_RESULT res = LUA_EvalFile(level->script_path);
        if (res.code != LUA_OK) {
            Console_ShowError("Lua level script error: %s", res.message);
        }
        LUA_FreeResult(&res);
    }

    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
}

void LUA_ReloadLevelScript(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr) {
        return;
    }
    // The level stays where it is, so the unload that would otherwise let go of
    // the last run is not coming.
    LUA_DropLevelScript();
    LUA_RunLevelScript(level);
}

REGISTER_LUA_CAPI(.shutdown = M_Shutdown)
