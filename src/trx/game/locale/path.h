#pragma once

#include <trx/game/locale/types.h>
#include <trx/game/paths.h>

// Works out the file a localized path names for a chain of languages: the
// names the chain's languages give it, then the names every language falls
// back to, each looked for in the languages' folders before as it is, and
// failing those the names any other language gives it. Returns nullptr where
// none is installed. The caller frees the result.
char *LocalePath_Resolve(
    const LOCALE_PATH *path, const LOCALE_CHAIN *chain, GAME_DYNAMIC_PATH kind);

// Frees the names a localized path holds, leaving it empty.
void LocalePath_Free(LOCALE_PATH *path);
