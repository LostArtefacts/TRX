#include <trx/game/level/settings.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/output.h>

#include <stddef.h>

#define M_DECLARED(field_)                                                     \
    .offset = offsetof(GF_LEVEL_SETTINGS, field_.value),                       \
    .present_offset = offsetof(GF_LEVEL_SETTINGS, field_.is_present),          \
    .declared_type =                                                           \
        Value_TypeOf(((GF_LEVEL_SETTINGS *)nullptr)->field_.value)

// Describes each setting's type, declared storage, config source, and fallback.
typedef struct {
    TRX_VALUE_TYPE type;
    size_t offset;
    size_t present_offset;
    TRX_VALUE_TYPE declared_type;
    // Null if no config source exists.
    const void *config_ptr;
    TRX_VALUE_TYPE config_type;
    // Used when no source supplies a value.
    TRX_VALUE fallback;
} M_SETTING_INFO;

// clang-format off
static const M_SETTING_INFO m_Settings[LEVEL_SETTING_NUMBER_OF] = {
#define X_SETTING(name_, field_, type_, config_)                               \
    [LEVEL_SETTING_##name_] = {                                                \
        .type = Value_TypeOf(((GF_LEVEL_SETTINGS *)nullptr)->field_.value),    \
        M_DECLARED(field_),                                                    \
        .config_ptr = &g_Config.config_,                                       \
        .config_type = Value_TypeOf(g_Config.config_),                         \
    },
#define X_SETTING_ENUM(name_, field_, enum_, fallback_)                        \
    [LEVEL_SETTING_##name_] = {                                                \
        .type = TVT_S32,                                                       \
        M_DECLARED(field_),                                                    \
        .fallback = { .type = TVT_S32, .as_int = fallback_ },                  \
    },
#include <trx/game/level/settings.def>
#undef X_SETTING
#undef X_SETTING_ENUM
};
// clang-format on

static struct {
    bool is_present;
    TRX_VALUE value;
} m_Overrides[LEVEL_SETTING_NUMBER_OF];

// Coerces `in` to the setting type and checks the range.
static RESULT M_Normalize(
    const M_SETTING_INFO *const info, const TRX_VALUE *const in,
    TRX_VALUE *const out)
{
    if (in->type == info->type) {
        *out = *in;
    } else if (!Value_Coerce(info->type, in, out)) {
        return FAIL(
            "expected %s, got %s", Value_TypeName(info->type),
            Value_TypeName(in->type));
    }

    const char *const error = Value_CheckRange(info->type, out);
    FAIL_IF(error != nullptr, "%s", error);
    return OK;
}

// Reads trusted engine storage as a normalized setting value.
static TRX_VALUE M_Read(
    const M_SETTING_INFO *const info, const TRX_VALUE_TYPE type,
    const void *const src)
{
    TRX_VALUE raw;
    Value_ReadPtr(type, src, &raw);
    TRX_VALUE out;
    RESULT result = M_Normalize(info, &raw, &out);
    ASSERT(IS_OK(result));
    IGNORE(result);
    return out;
}

// Returns a declared setting value from game flow storage.
static bool M_ReadDeclared(
    const M_SETTING_INFO *const info, const GF_LEVEL_SETTINGS *const settings,
    TRX_VALUE *const out)
{
    const char *const base = (const char *)settings;
    if (!*(const bool *)(base + info->present_offset)) {
        return false;
    }
    *out = M_Read(info, info->declared_type, base + info->offset);
    return true;
}

static TRX_VALUE M_Resolve(const LEVEL_SETTING setting, const bool held)
{
    ASSERT(setting >= 0 && setting < LEVEL_SETTING_NUMBER_OF);
    const M_SETTING_INFO *const info = &m_Settings[setting];
    if (held && m_Overrides[setting].is_present) {
        return m_Overrides[setting].value;
    }

    const GF_LEVEL *const level = GF_GetCurrentLevel();
    TRX_VALUE value;
    if (level != nullptr && M_ReadDeclared(info, &level->settings, &value)) {
        return value;
    }
    if (M_ReadDeclared(info, &g_GameFlow.settings, &value)) {
        return value;
    }

    if (info->config_ptr != nullptr) {
        return M_Read(info, info->config_type, info->config_ptr);
    }
    return info->fallback;
}

RESULT Level_SetDeclaredSetting(
    GF_LEVEL_SETTINGS *const settings, const LEVEL_SETTING setting,
    const TRX_VALUE *const value)
{
    ASSERT(settings != nullptr);
    ASSERT(setting >= 0 && setting < LEVEL_SETTING_NUMBER_OF);
    ASSERT(value != nullptr);

    const M_SETTING_INFO *const info = &m_Settings[setting];
    TRX_VALUE normalized;
    MUST(M_Normalize(info, value, &normalized));

    char *const base = (char *)settings;
    const char *const error =
        Value_WritePtr(info->declared_type, base + info->offset, &normalized);
    FAIL_IF(error != nullptr, "%s", error);
    *(bool *)(base + info->present_offset) = true;
    return OK;
}

TRX_VALUE Level_GetEffectiveSetting(const LEVEL_SETTING setting)
{
    return M_Resolve(setting, true);
}

TRX_VALUE Level_GetDeclaredSetting(const LEVEL_SETTING setting)
{
    return M_Resolve(setting, false);
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
    TRX_VALUE normalized;
    MUST(M_Normalize(info, value, &normalized));

    if (m_Overrides[setting].is_present
        && Value_Equal(&m_Overrides[setting].value, &normalized)) {
        return OK;
    }
    m_Overrides[setting].is_present = true;
    m_Overrides[setting].value = normalized;
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

RGB_888 Level_GetBackgroundColor(void)
{
    if (Level_GetEffectiveSetting(LEVEL_SETTING_FOG_TRANSPARENCY).as_bool) {
        return (RGB_888) { 0, 0, 0 };
    }
    return Level_GetDeclaredSetting(LEVEL_SETTING_FOG_COLOR).as_rgb;
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
