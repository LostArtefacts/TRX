#pragma once

#include <trx/core/colors.h>
#include <trx/game/game_flow/types.h>

RGB_888 Level_GetWaterColor(void);
RGBA_8888 Level_GetFogColor(void);

// Returns the fog draw color, regardless of source and transparency. Fog bulbs
// use this color as their seed.
RGB_888 Level_GetFogTint(void);

// The fog color a script is holding the level to, which outranks both the
// level's own color and the player's. Null means none is held, and the level
// is back to the color it carries. TR4 changes it mid-level through a flip
// effect, and the save carries what it was changed to.
const RGB_888 *Level_GetFogColorOverride(void);
void Level_SetFogColorOverride(const RGB_888 *color);
void Level_ResetFogColorOverride(void);
bool Level_AreFogBulbsEnabled(void);
float Level_GetFogStart(void);
float Level_GetFogEnd(void);
GF_DEATH_TILE Level_GetDeathTile(void);
