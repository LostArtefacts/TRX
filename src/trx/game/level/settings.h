#pragma once

#include <trx/core/colors.h>
#include <trx/core/result.h>
#include <trx/core/value.h>
#include <trx/game/game_flow/types.h>

// Lists settings a level can declare, a player can choose, and a script can
// override. Each setting resolves from script override, level, game flow root,
// config, then built-in default.
typedef enum {
    LEVEL_SETTING_FOG_START,
    LEVEL_SETTING_FOG_END,
    LEVEL_SETTING_FOG_TRANSPARENCY,
    LEVEL_SETTING_FOG_COLOR,
    LEVEL_SETTING_FOG_BULBS,
    LEVEL_SETTING_WATER_COLOR,
    LEVEL_SETTING_DEATH_TILE,
    LEVEL_SETTING_NUMBER_OF,
} LEVEL_SETTING;

// Returns the effective value resolved through the setting stack. The returned
// type is the setting type, whichever source supplies the value.
TRX_VALUE Level_GetEffectiveSetting(LEVEL_SETTING setting);

// Returns the script override. Null means no override is active. TR4 can change
// fog color during a level through a flip effect, and saves carry that value.
const TRX_VALUE *Level_GetSettingOverride(LEVEL_SETTING setting);

// Sets the script override to `value`, or clears it when `value` is null.
// Values are coerced to the setting type and refused when they do not fit.
RESULT Level_SetSettingOverride(LEVEL_SETTING setting, const TRX_VALUE *value);
void Level_ResetSettingOverride(LEVEL_SETTING setting);
void Level_ResetSettingOverrides(void);

RGB_888 Level_GetWaterColor(void);
RGBA_8888 Level_GetFogColor(void);

// Returns the fog draw color, regardless of source and transparency. Fog bulbs
// use this color as their seed.
RGB_888 Level_GetFogTint(void);

bool Level_AreFogBulbsEnabled(void);
float Level_GetFogStart(void);
float Level_GetFogEnd(void);
GF_DEATH_TILE Level_GetDeathTile(void);
