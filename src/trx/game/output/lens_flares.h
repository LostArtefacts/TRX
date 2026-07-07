#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>
#include <trx/game/items/types.h>

// TR4 lens flare: a gameflow-configured sun flare plus placeable LENS_FLARE
// objects, drawn as a chain of additive ghost sprites spread along the line
// from the flare through the screen center, with a full-screen flash when
// the flare sits close to the center.

void Output_LensFlares_Reset(void);
void Output_LensFlares_SetSun(XYZ_32 pos, RGB_888 color);

// Decays the full-screen flash; call once per logic tick.
void Output_LensFlares_Update(void);

// Stages the sun flare and any pending flash; call within the scene pass.
void Output_LensFlares_Draw(void);

// Stages a placeable LENS_FLARE object's flare.
bool Output_LensFlares_DrawObject(const ITEM *item);
