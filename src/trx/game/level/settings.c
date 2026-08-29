#include <trx/game/level/settings.h>

#include <trx/config.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/output.h>

static struct {
    bool is_present;
    RGB_888 value;
} m_FogColorOverride;

RGB_888 Level_GetWaterColor(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.water_color.is_present) {
        return level->settings.water_color.value;
    }
    return g_Config.visuals.water_color;
}

const RGB_888 *Level_GetFogColorOverride(void)
{
    return m_FogColorOverride.is_present ? &m_FogColorOverride.value : nullptr;
}

void Level_SetFogColorOverride(const RGB_888 *const color)
{
    if (color == nullptr) {
        Level_ResetFogColorOverride();
        return;
    }
    const RGB_888 held = m_FogColorOverride.value;
    if (m_FogColorOverride.is_present && held.r == color->r
        && held.g == color->g && held.b == color->b) {
        return;
    }
    m_FogColorOverride.is_present = true;
    m_FogColorOverride.value = *color;
    Output_ApplyLevelSettings();
}

void Level_ResetFogColorOverride(void)
{
    if (!m_FogColorOverride.is_present) {
        return;
    }
    m_FogColorOverride.is_present = false;
    Output_ApplyLevelSettings();
}

RGB_888 Level_GetFogTint(void)
{
    if (m_FogColorOverride.is_present) {
        return m_FogColorOverride.value;
    }
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.fog_color.is_present) {
        return level->settings.fog_color.value;
    }
    return g_Config.visuals.fog_color;
}

RGBA_8888 Level_GetFogColor(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    RGB_888 color = { 0, 0, 0 };
    uint8_t alpha = 255;
    if (m_FogColorOverride.is_present) {
        color = m_FogColorOverride.value;
    } else if (
        level != nullptr && level->settings.fog_transparency.is_present
        && level->settings.fog_transparency.value) {
        alpha = 0;
    } else if (level != nullptr && level->settings.fog_color.is_present) {
        color = level->settings.fog_color.value;
    } else if (g_Config.visuals.fog_transparency) {
        alpha = 0;
    } else {
        color = g_Config.visuals.fog_color;
    }
    return (RGBA_8888) { .r = color.r, .g = color.g, .b = color.b, .a = alpha };
}

bool Level_AreFogBulbsEnabled(void)
{
    if (!g_Config.visuals.enable_fog_bulbs) {
        return false;
    }
    // The OG disables volumetric fog bulbs on some levels (GF_TRAIN).
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.fog_bulbs.is_present) {
        return level->settings.fog_bulbs.value;
    }
    return true;
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

GF_DEATH_TILE Level_GetDeathTile(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->settings.death_tile.is_present) {
        return level->settings.death_tile.value;
    }

    if (g_GameFlow.settings.death_tile.is_present) {
        return g_GameFlow.settings.death_tile.value;
    }

    return GF_DEATH_TILE_LAVA;
}
