// A player's config of a handful of options, one of each shape that behaves
// differently. The options themselves are the real ones, holds and all; what is
// faked is everything around them - what options exist, how a value reads and
// writes as a string, and what saving means.

#include <fakes/config.h>

#include <harness/fake_calls.h>

#include <trx/config/priv.h>
#include <trx/config/registry.h>
#include <trx/core/enum_map.h>
#include <trx/core/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool m_EnableMusic;
static int32_t m_Fov;
static double m_Brightness;
static double m_MasterVolume;
static char *m_WaterColor;
static int32_t m_ShadowType;

// Whether anything the player chose has moved since the last update.
static bool m_PendingPersist;

static const CONFIG_OPTION_DESC m_Descs[] = {
    { .name = "audio.enable_music",
      .default_value = { .type = TVT_BOOL, .as_bool = true },
      .mirror = &m_EnableMusic },
    { .name = "visuals.fov",
      .default_value = { .type = TVT_S32, .as_int = 65 },
      .mirror = &m_Fov },
    { .name = "visuals.brightness",
      .default_value = { .type = TVT_DOUBLE, .as_num = 1.5 },
      .mirror = &m_Brightness },
    { .name = "audio.master_volume",
      .default_value = { .type = TVT_DOUBLE, .as_num = 1.0 },
      .mirror = &m_MasterVolume,
      .percent = true },
    { .name = "visuals.water_color",
      .default_value = { .type = TVT_STRING, .as_str = "ff0000" },
      .mirror = &m_WaterColor },
    { .name = "visuals.shadow_type",
      .default_value = { .type = TVT_ENUM, .as_int = 0 },
      .mirror = &m_ShadowType,
      .enum_map = "FAKE_SHADOW" },
};

static CONFIG_OPTION m_Options[ARRAY_SIZE(m_Descs)];
// What Config_GetOptions hands out: the same options, null terminated.
static CONFIG_OPTION *m_View[ARRAY_SIZE(m_Descs) + 1];

// The third value carries an underscore, which is what the console's
// dash-for-underscore spelling is about.
static void M_DefineEnums(void)
{
    static bool defined = false;
    if (defined) {
        return;
    }
    defined = true;
    EnumMap_Define(
        "FAKE_SHADOW", "SHADOW_CIRCLE", "enums/FAKE_SHADOW/circle", 0,
        "circle");
    EnumMap_Define(
        "FAKE_SHADOW", "SHADOW_SPRITE", "enums/FAKE_SHADOW/sprite", 1,
        "sprite");
    EnumMap_Define(
        "FAKE_SHADOW", "SHADOW_EXTRA_DARK", "enums/FAKE_SHADOW/extra_dark", 2,
        "extra_dark");
}

static void M_Reset(void)
{
    M_DefineEnums();
    for (size_t i = 0; i < ARRAY_SIZE(m_Descs); i++) {
        if (m_Options[i].name != nullptr) {
            Config_Option_Free(&m_Options[i]);
        }
        m_Options[i] = (CONFIG_OPTION) {};
        Config_Option_Init(&m_Options[i], &m_Descs[i]);
        m_View[i] = &m_Options[i];
    }
    m_View[ARRAY_SIZE(m_Descs)] = nullptr;
    m_PendingPersist = false;
}

// The value a string spells, borrowing the string for a string-typed option -
// the option copies what it is given.
static bool M_Parse(
    const CONFIG_OPTION *const option, const char *const new_value,
    TRX_VALUE *const out)
{
    TRX_VALUE value = { .type = option->value.type };
    switch (option->value.type) {
    case TVT_BOOL:
        if (strcmp(new_value, "true") != 0 && strcmp(new_value, "false") != 0) {
            return false;
        }
        value.as_bool = strcmp(new_value, "true") == 0;
        break;

    case TVT_S32: {
        char *end = nullptr;
        const long parsed = strtol(new_value, &end, 10);
        if (end == new_value || *end != '\0') {
            return false;
        }
        value.as_int = parsed;
        break;
    }

    case TVT_DOUBLE: {
        char *end = nullptr;
        const double parsed = strtod(new_value, &end);
        if (end == new_value || *end != '\0') {
            return false;
        }
        value.as_num = parsed;
        break;
    }

    case TVT_STRING:
        if (strlen(new_value) != 6) {
            return false;
        }
        value.as_str = new_value;
        break;

    case TVT_ENUM: {
        const int32_t parsed = EnumMap_Get(option->enum_map, new_value, -1);
        if (parsed == -1) {
            return false;
        }
        value.as_int = parsed;
        break;
    }

    default:
        return false;
    }

    *out = value;
    return true;
}

// Which option moved is nobody's business here - a test that cares about a
// write looks at the setting it landed on. Whether any of it was the player's
// own doing is, because that is what saving the file turns on.
void Config_ReportChange(const CONFIG_OPTION *const option, const bool persist)
{
    m_PendingPersist |= persist;
}

CONFIG_OPTION *const *Config_GetOptions(void)
{
    return m_View;
}

CONFIG_OPTION *Config_FindOption(const char *const path)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_Options); i++) {
        if (strcmp(m_Options[i].name, path) == 0) {
            return &m_Options[i];
        }
    }
    return nullptr;
}

CONFIG_OPTION *Config_FindOptionByMirror(const void *const mirror)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_Options); i++) {
        if (m_Options[i].mirror == mirror) {
            return &m_Options[i];
        }
    }
    return nullptr;
}

bool Config_Update(void)
{
    if (m_PendingPersist) {
        m_PendingPersist = false;
        FAKE_RECORD("config_write");
    }
    return true;
}

const char *Config_Option_GetValueAsString(
    const CONFIG_OPTION *const option, const bool human_readable)
{
    static char buf[64];
    switch (option->value.type) {
    case TVT_BOOL:
        return option->value.as_bool ? "true" : "false";
    case TVT_S32:
        snprintf(buf, sizeof(buf), "%d", (int32_t)option->value.as_int);
        return buf;
    case TVT_DOUBLE:
        snprintf(buf, sizeof(buf), "%g", option->value.as_num);
        return buf;
    case TVT_STRING:
        return option->value.as_str != nullptr ? option->value.as_str : "";
    case TVT_ENUM:
        return EnumMap_ToString(option->enum_map, option->value.as_int);
    default:
        return "";
    }
}

bool Config_Option_SetFromString(
    CONFIG_OPTION *const option, const char *const new_value, const bool force)
{
    if (!force && Config_Option_IsHeld(option)) {
        return false;
    }
    TRX_VALUE value;
    if (!M_Parse(option, new_value, &value)) {
        return false;
    }
    Config_Option_Write(option, &value);
    return true;
}

bool Config_Option_PushHoldFromString(
    CONFIG_OPTION *const option, const char *const value,
    const CONFIG_HOLD_SOURCE source)
{
    TRX_VALUE parsed;
    if (Config_Option_IsEnforced(option) || !M_Parse(option, value, &parsed)) {
        return false;
    }
    return Config_Option_PushHold(option, &parsed, source);
}

FAKE_ON_RESET(M_Reset)

// The game flow enforcing a setting is a hold like any other, so this puts one
// on every option rather than standing a flag beside them.
void FakeConfig_SetEnforced(const bool enforced)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_Options); i++) {
        if (enforced) {
            Config_Option_PushHold(
                &m_Options[i], &m_Options[i].value, CONFIG_HOLD_GAME_FLOW);
        } else {
            Config_Option_PopHold(&m_Options[i]);
        }
    }
}
