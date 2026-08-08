// Choosing which of the shipped languages fits the one the player's system is
// set to.
#pragma once

#include <trx/core/vector.h>

// The code from `available` that best fits `preferred`, or nullptr where none
// of them does. Both are vectors of char *: `available` holds the codes the
// strings files were discovered under, `preferred` the player's locales from
// most wanted to least. A region is honoured before it is discarded, so pt-br
// is answered with pt-br where that ships, with pt where it does not, and with
// another Portuguese only as a last resort.
// The returned pointer belongs to `available`.
const char *GameStringLang_MatchPreferred(
    const VECTOR *available, const VECTOR *preferred);
