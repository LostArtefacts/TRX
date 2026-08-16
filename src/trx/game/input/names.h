#pragma once

#include <stdint.h>

// Glyph joining the keys of a combination. Unlike a plain "+", it is centered
// on the key icons.
#define INPUT_COMBO_SEPARATOR "\\{icon plus}"

// Spell out one binding as the text a UI draws for it. Several keys read as a
// combination and are joined with INPUT_COMBO_SEPARATOR; a key with no name
// drops out and takes its separator with it. Returns nullptr for no keys, and
// the name itself for one. Joined text lives in a static buffer.
const char *Input_JoinKeyNames(const char *const *names, int32_t count);
