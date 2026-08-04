// The resolver LUA_RunGameScript reaches for. A test that exercises the level
// scripts runs no game script, and standing the real one up would bring the
// path policy with it.

#include <trx/game/shell/paths.h>

const char *TRXPath_PeekResolve(
    const TRX_DYNAMIC_PATH path, const char *const rel)
{
    return nullptr;
}
