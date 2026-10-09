#pragma once

#include <trx/game/locale/types.h>
#include <trx/game/paths.h>

// Lookups along a chain of languages. Nothing here reads the settings: the
// caller says which languages it wants.

typedef char *(*LOCALE_FIND_FUNC)(const char *path, void *user_data);

// A regional code is followed by its language, so "pt-BR" gives pt-br, pt.
// nullptr or "" gives an empty chain.
LOCALE_CHAIN LocaleChain_FromCode(const char *code);

// Hands func the path moved into the folder of each language in turn, so that
// "audio/011.wav" is tried as "audio/it/011.wav", and last the path as it is,
// until func returns something. Returns what func returned, or nullptr where
// it never did.
char *LocaleChain_Find(
    const LOCALE_CHAIN *chain, const char *path, LOCALE_FIND_FUNC func,
    void *user_data);

// LocaleChain_Find over GamePath_PeekResolve. The caller frees the result.
char *LocaleChain_Resolve(
    const LOCALE_CHAIN *chain, GAME_DYNAMIC_PATH path, const char *rel);
