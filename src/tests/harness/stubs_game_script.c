// The resolver require() and LUA_RunGameScript reach for. Standing the real
// one up would bring the path policy with it, so the one here looks in a
// single directory per source that a test points it at.

#include <harness/stubs_game_script.h>

#include <trx/core/memory.h>
#include <trx/game/lua/startup.h>
#include <trx/game/paths.h>

#include <stdio.h>

static const char *m_ScriptDirs[GAME_DYNAMIC_PATH_NUMBER_OF] = {};
static const char *m_OwnDir = nullptr;

void FakeGameScript_SetScriptDir(
    const GAME_DYNAMIC_PATH path, const char *const dir)
{
    m_ScriptDirs[path] = dir;
}

void FakeGameScript_SetOwnDir(const char *const dir)
{
    m_OwnDir = dir;
}

const char *LUA_GetStartupScriptDir(void)
{
    return m_OwnDir;
}

// Return the path when it names an openable file.
char *GamePath_ResolveCase(const char *const path)
{
    FILE *const fp = fopen(path, "rb");
    if (fp == nullptr) {
        return nullptr;
    }
    fclose(fp);
    return Memory_DupStr(path);
}

const char *GamePath_PeekResolve(
    const GAME_DYNAMIC_PATH path, const char *const rel)
{
    const char *const dir = m_ScriptDirs[path];
    if (dir == nullptr) {
        return nullptr;
    }

    static char resolved[512];
    snprintf(resolved, sizeof(resolved), "%s/%s", dir, rel);
    FILE *const fp = fopen(resolved, "rb");
    if (fp == nullptr) {
        return nullptr;
    }
    fclose(fp);
    return resolved;
}
