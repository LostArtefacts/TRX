#pragma once

#include <stdint.h>

// The shipped games directory, passed in by the build so the font metrics can
// be read from each game's font.bin.
#ifndef TEST_SHIP_GAMES_DIR
    #error TEST_SHIP_GAMES_DIR must be defined
#endif

// Point the font metrics at a game's glyph table and set g_TRVersion to match.
void FakeUI_SetGame(int32_t tr_version);

void FakeUI_SetViewport(int32_t width, int32_t height);

// Name the keys every "\{input <role>}" placeholder stands for; nullptr leaves
// the roles unbound.
void FakeUI_SetKeyName(const char *key_name);

void FakeUI_ResetKeyName(void);

void FakeUI_Shutdown(void);
