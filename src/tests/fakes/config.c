// A player's config of a handful of options, one of each shape that behaves
// differently. The override stack underneath is the real one; only the facade
// around it - what options exist, how a string becomes a value, what writing to
// disk means - is faked.

#include <fakes/config.h>

#include <harness/fake_calls.h>

#include <trx/config/override.h>
#include <trx/core/enum_map.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool m_EnableMusic;
static int32_t m_Fov;
static double m_Brightness;
static double m_MasterVolume;
static char *m_WaterColor;
static int32_t m_ShadowType;
static bool m_Enforced;

static const CONFIG_OPTION m_Options[] = {
    { .name = "audio.enable_music",
      .type = TVT_BOOL,
      .target = &m_EnableMusic },
    { .name = "visuals.fov", .type = TVT_S32, .target = &m_Fov },
    { .name = "visuals.brightness",
      .type = TVT_DOUBLE,
      .target = &m_Brightness },
    { .name = "audio.master_volume",
      .type = TVT_DOUBLE,
      .target = &m_MasterVolume,
      .percent = true },
    { .name = "visuals.water_color",
      .type = TVT_STRING,
      .target = &m_WaterColor },
    { .name = "visuals.shadow_type",
      .type = TVT_ENUM,
      .target = &m_ShadowType,
      .param = "FAKE_SHADOW" },
    { nullptr },
};

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
    ConfigOverride_Clear();
    m_Enforced = false;
    m_EnableMusic = true;
    m_Fov = 65;
    m_Brightness = 1.5;
    m_MasterVolume = 1.0;
    free(m_WaterColor);
    m_WaterColor = strdup("ff0000");
    m_ShadowType = 0;
}

const CONFIG_OPTION *Config_GetOptionMap(void)
{
    return m_Options;
}

const CONFIG_OPTION *Config_GetOptionByPath(const char *const path)
{
    for (const CONFIG_OPTION *o = m_Options; o->name != nullptr; o++) {
        if (strcmp(o->name, path) == 0) {
            return o;
        }
    }
    return nullptr;
}

const CONFIG_OPTION *Config_GetOption(const void *const target)
{
    for (const CONFIG_OPTION *o = m_Options; o->name != nullptr; o++) {
        if (o->target == target) {
            return o;
        }
    }
    return nullptr;
}

bool Config_IsOptionEnforced(const void *const target)
{
    return m_Enforced;
}

bool Config_Update(void)
{
    FAKE_RECORD("config_write");
    return true;
}

const char *Config_GetOptionValueAsString(
    const CONFIG_OPTION *const option, const bool human_readable)
{
    static char buf[64];
    switch (option->type) {
    case TVT_BOOL:
        return *(const bool *)option->target ? "true" : "false";
    case TVT_S32:
        snprintf(buf, sizeof(buf), "%d", *(const int32_t *)option->target);
        return buf;
    case TVT_DOUBLE:
        snprintf(buf, sizeof(buf), "%g", *(const double *)option->target);
        return buf;
    case TVT_STRING: {
        const char *const value = *(const char *const *)option->target;
        return value != nullptr ? value : "";
    }
    case TVT_ENUM:
        return EnumMap_ToString(
            (const char *)option->param, *(const int32_t *)option->target);
    default:
        return "";
    }
}

bool Config_SetOptionValueFromStringForce(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    switch (option->type) {
    case TVT_BOOL:
        if (strcmp(new_value, "true") == 0 || strcmp(new_value, "false") == 0) {
            *(bool *)option->target = strcmp(new_value, "true") == 0;
            return true;
        }
        return false;

    case TVT_S32: {
        char *end = nullptr;
        const long parsed = strtol(new_value, &end, 10);
        if (end == new_value || *end != '\0') {
            return false;
        }
        *(int32_t *)option->target = (int32_t)parsed;
        return true;
    }

    case TVT_DOUBLE: {
        char *end = nullptr;
        const double parsed = strtod(new_value, &end);
        if (end == new_value || *end != '\0') {
            return false;
        }
        *(double *)option->target = parsed;
        return true;
    }

    case TVT_STRING: {
        if (strlen(new_value) != 6) {
            return false;
        }
        char **const p = (char **)option->target;
        free(*p);
        *p = strdup(new_value);
        return true;
    }

    case TVT_ENUM: {
        const int32_t parsed =
            EnumMap_Get((const char *)option->param, new_value, -1);
        if (parsed == -1) {
            return false;
        }
        *(int32_t *)option->target = parsed;
        return true;
    }

    default:
        return false;
    }
}

bool Config_SetOptionValueFromString(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    if (m_Enforced) {
        return false;
    }
    return Config_SetOptionValueFromStringForce(option, new_value);
}

bool Config_RestoreOptionDefaultForce(const void *const target)
{
    if (target == &m_EnableMusic) {
        m_EnableMusic = true;
    } else if (target == &m_Fov) {
        m_Fov = 65;
    } else if (target == &m_Brightness) {
        m_Brightness = 1.5;
    } else if (target == &m_MasterVolume) {
        m_MasterVolume = 1.0;
    } else if (target == &m_WaterColor) {
        free(m_WaterColor);
        m_WaterColor = strdup("ff0000");
    } else if (target == &m_ShadowType) {
        m_ShadowType = 0;
    } else {
        return false;
    }
    return true;
}

bool Config_RestoreOptionDefault(const void *const target)
{
    if (m_Enforced) {
        return false;
    }
    return Config_RestoreOptionDefaultForce(target);
}

FAKE_ON_RESET(M_Reset)

void FakeConfig_SetEnforced(const bool enforced)
{
    m_Enforced = enforced;
}
