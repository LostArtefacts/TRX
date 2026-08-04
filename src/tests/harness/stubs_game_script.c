// The resolver require() and LUA_RunGameScript reach for. Standing the real
// one up would bring the path policy with it, so the one here looks in a
// single directory per source that a test points it at.

#include <harness/stubs_game_script.h>

#include <trx/game/shell/paths.h>

#include <stdio.h>

static const char *m_ScriptDirs[TRX_DYNAMIC_PATH_NUMBER_OF] = {};

void FakeGameScript_SetScriptDir(
    const TRX_DYNAMIC_PATH path, const char *const dir)
{
    m_ScriptDirs[path] = dir;
}

const char *TRXPath_PeekResolve(
    const TRX_DYNAMIC_PATH path, const char *const rel)
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
