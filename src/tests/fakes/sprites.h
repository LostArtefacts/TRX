#pragma once

// Provides a sprite object with known sprite dimensions for drawing tests.

#include <trx/game/objects/ids.h>

#include <stdint.h>

// Assigns the specified number of sprites with the specified dimensions and
// marks the object as loaded, with each sprite centred on its drawing
// position.
void FakeSprites_Define(
    OBJECT_ID object_id, int32_t sprite_count, int32_t width, int32_t height);

void FakeSprites_Forget(void);
