#pragma once

// Reads the glyph metrics out of a shipped font.bin, which is what the game
// loads them from. Only the sprite textures and the sprite sequences that name
// the two fonts are of interest; everything else in the injection is skipped.

#include <trx/game/output/types.h>

#include <stdint.h>

#define FONT_BIN_FONT_COUNT 2

typedef struct {
    SPRITE_TEXTURE *sprites;
    int32_t sprite_count;
    // Where each font's sprites start, and how many it has. font.bin carries
    // the default font first and the small font second.
    int32_t font_base[FONT_BIN_FONT_COUNT];
    int32_t font_count[FONT_BIN_FONT_COUNT];
} FONT_BIN;

// Returns false if the file is missing or is not an injection this can read.
bool FontBin_Load(const char *path, FONT_BIN *out);
void FontBin_Free(FONT_BIN *font_bin);
