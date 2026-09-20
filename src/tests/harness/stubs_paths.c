// What game/paths.c reaches for outside itself: the executable's directory,
// which the shell reports through SDL, and the mod lookup behind the mod
// chain. SDL is not linked here, and the mods come from the game flow, which
// is not what a path is being tested for.

#include <harness/stubs_paths.h>

#include <trx/core/shell.h>
#include <trx/game/shell/mod.h>
#include <trx/version.h>

#include <stdlib.h>
#include <string.h>

const SHELL_MOD *Shell_GetModByName(const char *const name)
{
    return nullptr;
}

char *Shell_GetBasePath(void)
{
    return strdup(FAKE_BASE_PATH "/");
}

// Which game the paths expand %tr_version% to. The version lives with the
// engine's globals; a path is tested against one game at a time.
int32_t g_TRVersion = 1;
