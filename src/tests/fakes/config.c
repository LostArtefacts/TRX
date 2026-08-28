// A player's config of a handful of options, one of each shape that behaves
// differently. The registry and the options are the real ones, holds and all;
// what is faked is which options exist, how a value reads and writes as a
// string, and what saving means.

#include <fakes/config.h>

#include <harness/fake_calls.h>

#include <trx/config.h>
#include <trx/config/common.h>
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
static RGB_888 m_WaterColor;
static int32_t m_ShadowType;

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
      .default_value = { .type = TVT_RGB_888, .as_rgb = { 0xFF, 0x00, 0x00 } },
      .mirror = &m_WaterColor },
    { .name = "visuals.shadow_type",
      .default_value = { .type = TVT_ENUM, .as_int = 0 },
      .mirror = &m_ShadowType,
      .enum_map = "FAKE_SHADOW" },
    // Widget layout listens to these two options.
    { .name = "ui.text_scale",
      .default_value = { .type = TVT_FLOAT, .as_num = 1.0 },
      .mirror = &g_ConfigStorage.ui.text_scale,
      .percent = true },
    { .name = "ui.bar_scale",
      .default_value = { .type = TVT_FLOAT, .as_num = 1.0 },
      .mirror = &g_ConfigStorage.ui.bar_scale,
      .percent = true },
    { .name = "ui.enable_smooth_bars",
      .default_value = { .type = TVT_BOOL, .as_bool = true },
      .mirror = &g_ConfigStorage.ui.enable_smooth_bars },
};

static int32_t m_Listener = -1;

// The third value carries an underscore, which is what the console's
// dash-for-underscore spelling is about.
static void M_DefineEnums(void)
{
    static bool defined = false;
    if (defined) {
        return;
    }
    defined = true;
    EnumMap_Define("FAKE_SHADOW", "SHADOW_CIRCLE", 0, "circle");
    EnumMap_Define("FAKE_SHADOW", "SHADOW_SPRITE", 1, "sprite");
    EnumMap_Define("FAKE_SHADOW", "SHADOW_EXTRA_DARK", 2, "extra_dark");
}

// What the settings file would have taken. A change a hold applies is nobody's
// to save, so only the ones the player made are recorded.
static void M_RecordWrite(const EVENT *const event, void *const user_data)
{
    if (((const CONFIG_CHANGE *)event->data)->persist) {
        FAKE_RECORD("config_write");
    }
}

static void M_Reset(void)
{
    M_DefineEnums();
    Config_DropAllOptions();
    for (size_t i = 0; i < ARRAY_SIZE(m_Descs); i++) {
        Config_Register(&m_Descs[i]);
    }
    if (m_Listener < 0) {
        m_Listener = Config_SubscribeChanges(M_RecordWrite, nullptr);
    }
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

    case TVT_FLOAT:
    case TVT_DOUBLE: {
        char *end = nullptr;
        const double parsed = strtod(new_value, &end);
        if (end == new_value || *end != '\0') {
            return false;
        }
        value.as_num = parsed;
        break;
    }

    case TVT_RGB_888:
        if (!Value_Parse(TVT_RGB_888, nullptr, new_value, &value)) {
            return false;
        }
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
    case TVT_FLOAT:
    case TVT_DOUBLE:
        snprintf(buf, sizeof(buf), "%g", option->value.as_num);
        return buf;
    case TVT_RGB_888:
        return Value_Format(TVT_RGB_888, nullptr, &option->value, false);
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
    for (CONFIG_OPTION *const *option = Config_GetOptions(); *option != nullptr;
         option++) {
        if (enforced) {
            Config_Option_PushHold(
                *option, &(*option)->value, CONFIG_HOLD_GAME_FLOW);
        } else {
            Config_Option_PopHold(*option);
        }
    }
}
