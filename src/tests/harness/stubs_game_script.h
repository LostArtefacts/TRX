#pragma once

#include <trx/game/paths.h>

// Points one of the script resolvers at a directory, so a test can hand
// require() real files. A source with no directory set resolves nothing, which
// is what a test running no scripts wants.
void FakeGameScript_SetScriptDir(GAME_DYNAMIC_PATH path, const char *dir);
