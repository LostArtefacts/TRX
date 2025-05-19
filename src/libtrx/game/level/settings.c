#include "game/level/settings.h"

#include "config.h"
#include "game/game_flow.h"
#include "game/game_flow/vars.h"

RGB_888 Level_GetWaterColor(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.water_color.is_present) {
        return level->settings.water_color.value;
    }
    return g_Config.visuals.water_color;
}

float Level_GetFogStart(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.fog_start.is_present) {
        return level->settings.fog_start.value;
    }
    return g_Config.visuals.fog_start;
}

float Level_GetFogEnd(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.fog_end.is_present) {
        return level->settings.fog_end.value;
    }
    return g_Config.visuals.fog_end;
}

const GF_AMBIENT_DATA *Level_GetAmbientData(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.ambient_tracks.is_present) {
        return &level->settings.ambient_tracks;
    }
    if (g_GameFlow.settings.ambient_tracks.is_present) {
        return &g_GameFlow.settings.ambient_tracks;
    }
    return nullptr;
}
