#pragma once

#include <trx/game/effects/types.h>

// Chooses how an effect is lit.
typedef struct {
    // Lights a mesh effect by its room, rather than by its shade.
    bool room_light;
    // Lowers the shade of the light by this much, which shows the effect
    // brighter.
    int32_t brightness;
} EFFECT_DRAW_DESC;

// Draws an effect as a sprite or as a mesh, as its object has it. Returns
// true, as the effect is always drawn.
bool Effect_Draw(const EFFECT *effect);
bool Effect_DrawEx(const EFFECT *effect, const EFFECT_DRAW_DESC *desc);
