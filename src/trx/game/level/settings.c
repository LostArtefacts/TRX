#include <trx/game/level/settings.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/output.h>

#include <stddef.h>

#define M_LEVEL(field_)                                                        \
    .level_offset = offsetof(GF_LEVEL_SETTINGS, field_.value),                 \
    .level_present_offset = offsetof(GF_LEVEL_SETTINGS, field_.is_present),    \
    .level_type = Value_TypeOf(((GF_LEVEL_SETTINGS *)nullptr)->field_.value)

#define M_CONFIG(field_)                                                       \
    .config_ptr = &g_Config.field_, .config_type = Value_TypeOf(g_Config.field_)

// Describes each setting's type, declared storage, config source, and fallback.
typedef struct {
    TRX_VALUE_TYPE type;
    size_t level_offset;
    size_t level_present_offset;
    TRX_VALUE_TYPE level_type;
    // Null if no config source exists.
    const void *config_ptr;
    TRX_VALUE_TYPE config_type;
    // Used when no source supplies a value.
    TRX_VALUE fallback;
} M_SETTING_INFO;

static const M_SETTING_INFO m_Settings[LEVEL_SETTING_NUMBER_OF] = {
    [LEVEL_SETTING_FOG_START] = {
        .type = TVT_FLOAT,
        M_LEVEL(fog_start),
        M_CONFIG(visuals.fog_start),
    },
    [LEVEL_SETTING_FOG_END] = {
        .type = TVT_FLOAT,
        M_LEVEL(fog_end),
        M_CONFIG(visuals.fog_end),
    },
    [LEVEL_SETTING_FOG_TRANSPARENCY] = {
        .type = TVT_BOOL,
        M_LEVEL(fog_transparency),
        M_CONFIG(visuals.fog_transparency),
    },
    [LEVEL_SETTING_FOG_COLOR] = {
        .type = TVT_RGB_888,
        M_LEVEL(fog_color),
        M_CONFIG(visuals.fog_color),
    },
    [LEVEL_SETTING_FOG_BULBS] = {
        .type = TVT_BOOL,
        M_LEVEL(fog_bulbs),
        M_CONFIG(visuals.enable_fog_bulbs),
    },
    [LEVEL_SETTING_WATER_COLOR] = {
        .type = TVT_RGB_888,
        M_LEVEL(water_color),
        M_CONFIG(visuals.water_color),
    },
    [LEVEL_SETTING_DEATH_TILE] = {
        .type = TVT_S32,
        M_LEVEL(death_tile),
        .fallback = { .type = TVT_S32, .as_int = GF_DEATH_TILE_LAVA },
    },
};

static struct {
    bool is_present;
    TRX_VALUE value;
} m_Overrides[LEVEL_SETTING_NUMBER_OF];

// Reads the value at `src` as `type` and brings it into the type the setting is
// declared with.
static bool M_Read(
    const M_SETTING_INFO *const info, const TRX_VALUE_TYPE type,
    const void *const src, TRX_VALUE *const out)
{
    TRX_VALUE raw;
    Value_ReadPtr(type, src, &raw);
    if (raw.type == info->type) {
        *out = raw;
        return true;
    }
    return Value_Coerce(info->type, &raw, out);
}

TRX_VALUE Level_GetEffectiveSetting(const LEVEL_SETTING setting)
{
    ASSERT(setting >= 0 && setting < LEVEL_SETTING_NUMBER_OF);
    const M_SETTING_INFO *const info = &m_Settings[setting];
    if (m_Overrides[setting].is_present) {
        return m_Overrides[setting].value;
    }

    const GF_LEVEL *const level = GF_GetCurrentLevel();
    const GF_LEVEL_SETTINGS *const declared =
        level != nullptr ? &level->settings : &g_GameFlow.settings;
    const char *const base = (const char *)declared;
    TRX_VALUE value;
    if (*(const bool *)(base + info->level_present_offset)
        && M_Read(info, info->level_type, base + info->level_offset, &value)) {
        return value;
    }

    if (info->config_ptr != nullptr
        && M_Read(info, info->config_type, info->config_ptr, &value)) {
        return value;
    }
    return info->fallback;
}

const TRX_VALUE *Level_GetSettingOverride(const LEVEL_SETTING setting)
{
    ASSERT(setting >= 0 && setting < LEVEL_SETTING_NUMBER_OF);
    return m_Overrides[setting].is_present ? &m_Overrides[setting].value
                                           : nullptr;
}

RESULT Level_SetSettingOverride(
    const LEVEL_SETTING setting, const TRX_VALUE *const value)
{
    ASSERT(setting >= 0 && setting < LEVEL_SETTING_NUMBER_OF);
    if (value == nullptr) {
        Level_ResetSettingOverride(setting);
        return OK;
    }

    const M_SETTING_INFO *const info = &m_Settings[setting];
    TRX_VALUE held = *value;
    if (held.type != info->type && !Value_Coerce(info->type, value, &held)) {
        return FAIL(
            "expected %s, got %s", Value_TypeName(info->type),
            Value_TypeName(value->type));
    }
    const char *const error = Value_CheckRange(info->type, &held);
    if (error != nullptr) {
        return FAIL("%s", error);
    }

    if (m_Overrides[setting].is_present
        && Value_Equal(&m_Overrides[setting].value, &held)) {
        return OK;
    }
    m_Overrides[setting].is_present = true;
    m_Overrides[setting].value = held;
    Output_ApplyLevelSettings();
    return OK;
}

void Level_ResetSettingOverride(const LEVEL_SETTING setting)
{
    ASSERT(setting >= 0 && setting < LEVEL_SETTING_NUMBER_OF);
    if (!m_Overrides[setting].is_present) {
        return;
    }
    m_Overrides[setting].is_present = false;
    Output_ApplyLevelSettings();
}

void Level_ResetSettingOverrides(void)
{
    for (int32_t i = 0; i < LEVEL_SETTING_NUMBER_OF; i++) {
        Level_ResetSettingOverride((LEVEL_SETTING)i);
    }
}

RGB_888 Level_GetWaterColor(void)
{
    return Level_GetEffectiveSetting(LEVEL_SETTING_WATER_COLOR).as_rgb;
}

RGB_888 Level_GetFogTint(void)
{
    return Level_GetEffectiveSetting(LEVEL_SETTING_FOG_COLOR).as_rgb;
}

RGBA_8888 Level_GetFogColor(void)
{
    if (Level_GetSettingOverride(LEVEL_SETTING_FOG_COLOR) == nullptr
        && Level_GetEffectiveSetting(LEVEL_SETTING_FOG_TRANSPARENCY).as_bool) {
        return (RGBA_8888) { .r = 0, .g = 0, .b = 0, .a = 0 };
    }
    const RGB_888 color = Level_GetFogTint();
    return (RGBA_8888) { .r = color.r, .g = color.g, .b = color.b, .a = 255 };
}

bool Level_AreFogBulbsEnabled(void)
{
    return Level_GetEffectiveSetting(LEVEL_SETTING_FOG_BULBS).as_bool;
}

float Level_GetFogStart(void)
{
    return Level_GetEffectiveSetting(LEVEL_SETTING_FOG_START).as_num;
}

float Level_GetFogEnd(void)
{
    return Level_GetEffectiveSetting(LEVEL_SETTING_FOG_END).as_num;
}

GF_DEATH_TILE Level_GetDeathTile(void)
{
    return (GF_DEATH_TILE)Level_GetEffectiveSetting(LEVEL_SETTING_DEATH_TILE)
        .as_int;
}
