// A player's config of four options, one of each shape that behaves
// differently. The override stack underneath is the real one; only the facade
// around it - what options exist, how a string becomes a value, what writing to
// disk means - is faked.

#include "fake_engine_config.h"

#include <trx/config/override.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FAKE_CONFIG_CALLS g_FakeConfigCalls;

static bool m_EnableMusic;
static int32_t m_Fov;
static double m_Brightness;
static char *m_WaterColor;
static bool m_Enforced;

static const CONFIG_OPTION m_Options[] = {
    { .name = "audio.enable_music",
      .type = COT_BOOL,
      .target = &m_EnableMusic },
    { .name = "visuals.fov", .type = COT_INT32, .target = &m_Fov },
    { .name = "visuals.brightness",
      .type = COT_DOUBLE,
      .target = &m_Brightness },
    { .name = "visuals.water_color",
      .type = COT_STRING,
      .target = &m_WaterColor },
    { nullptr },
};

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
    g_FakeConfigCalls.config_writes++;
    return true;
}

const char *Config_GetOptionValueAsString(
    const CONFIG_OPTION *const option, const bool human_readable)
{
    static char buf[64];
    switch (option->type) {
    case COT_BOOL:
        return *(const bool *)option->target ? "true" : "false";
    case COT_INT32:
        snprintf(buf, sizeof(buf), "%d", *(const int32_t *)option->target);
        return buf;
    case COT_DOUBLE:
        snprintf(buf, sizeof(buf), "%g", *(const double *)option->target);
        return buf;
    case COT_STRING: {
        const char *const value = *(const char *const *)option->target;
        return value != nullptr ? value : "";
    }
    default:
        return "";
    }
}

bool Config_SetOptionValueFromString(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    if (m_Enforced) {
        return false;
    }
    switch (option->type) {
    case COT_BOOL:
        if (strcmp(new_value, "true") == 0 || strcmp(new_value, "false") == 0) {
            *(bool *)option->target = strcmp(new_value, "true") == 0;
            return true;
        }
        return false;

    case COT_INT32: {
        char *end = nullptr;
        const long parsed = strtol(new_value, &end, 10);
        if (end == new_value || *end != '\0') {
            return false;
        }
        *(int32_t *)option->target = (int32_t)parsed;
        return true;
    }

    case COT_DOUBLE: {
        char *end = nullptr;
        const double parsed = strtod(new_value, &end);
        if (end == new_value || *end != '\0') {
            return false;
        }
        *(double *)option->target = parsed;
        return true;
    }

    case COT_STRING: {
        if (strlen(new_value) != 6) {
            return false;
        }
        char **const p = (char **)option->target;
        free(*p);
        *p = strdup(new_value);
        return true;
    }

    default:
        return false;
    }
}

void FakeConfig_Reset(void)
{
    ConfigOverride_Clear();
    g_FakeConfigCalls = (FAKE_CONFIG_CALLS) {};
    m_Enforced = false;
    m_EnableMusic = true;
    m_Fov = 65;
    m_Brightness = 1.5;
    free(m_WaterColor);
    m_WaterColor = strdup("ff0000");
}

void FakeConfig_SetEnforced(const bool enforced)
{
    m_Enforced = enforced;
}
