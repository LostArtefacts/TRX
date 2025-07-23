#include "colors.h"
#include "config/option.h"
#include "config/types.h"
#include "config/vars.h"
#include "enum_map.h"
#include "utils.h"

#define X_CFG_BOOL(target_, default_value_)                                    \
    { .name = QUOTE(target_),                                                  \
      .type = COT_BOOL,                                                        \
      .target = &g_Config.target_,                                             \
      .default_value = &(bool) { default_value_ },                             \
      .param = nullptr },

#define X_CFG_INT32(target_, default_value_)                                   \
    { .name = QUOTE(target_),                                                  \
      .type = COT_INT32,                                                       \
      .target = &g_Config.target_,                                             \
      .default_value = &(int32_t) { default_value_ },                          \
      .param = nullptr },

#define X_CFG_FLOAT(target_, default_value_)                                   \
    { .name = QUOTE(target_),                                                  \
      .type = COT_FLOAT,                                                       \
      .target = &g_Config.target_,                                             \
      .default_value = &(float) { default_value_ },                            \
      .param = nullptr },

#define X_CFG_FLOAT_PERCENT(target_, default_value_)                           \
    { .name = QUOTE(target_),                                                  \
      .type = COT_FLOAT_PERCENT,                                               \
      .target = &g_Config.target_,                                             \
      .default_value = &(float) { default_value_ },                            \
      .param = nullptr },

#define X_CFG_DOUBLE(target_, default_value_)                                  \
    { .name = QUOTE(target_),                                                  \
      .type = COT_DOUBLE,                                                      \
      .target = &g_Config.target_,                                             \
      .default_value = &(double) { default_value_ },                           \
      .param = nullptr },

#define X_CFG_ENUM(target_, default_value_, enum_map)                          \
    X_CFG_ENUM_EX(QUOTE(target_), target_, default_value_, enum_map)

#define X_CFG_ENUM_EX(name_, target_, default_value_, enum_map)                \
    { .name = name_,                                                           \
      .type = COT_ENUM,                                                        \
      .target = &g_Config.target_,                                             \
      .default_value = &(int32_t) { default_value_ },                          \
      .param = ENUM_MAP_NAME(enum_map) },

#define X_CFG_RGB888(target_, default_r, default_g, default_b)                 \
    { .name = QUOTE(target_),                                                  \
      .type = COT_RGB888,                                                      \
      .target = &g_Config.target_,                                             \
      .default_value = &(RGB_888) { default_r, default_g, default_b },         \
      .param = nullptr },

#define X_CFG_STRING(target_, default_value_)                                  \
    { .name = QUOTE(target_),                                                  \
      .type = COT_STRING,                                                      \
      .target = &g_Config.target_,                                             \
      .default_value = default_value_,                                         \
      .param = nullptr },

static const CONFIG_OPTION m_ConfigOptionMap[] = {
#include "map.def"
    {}, // sentinel
};

#undef X_CFG_BOOL
#undef X_CFG_INT32
#undef X_CFG_FLOAT
#undef X_CFG_FLOAT_PERCENT
#undef X_CFG_DOUBLE
#undef X_CFG_ENUM
#undef X_CFG_ENUM_EX
#undef X_CFG_RGB888
#undef X_CFG_STRING

const CONFIG_OPTION *Config_GetOptionMap(void)
{
    return m_ConfigOptionMap;
}

const char *Config_ResolveOptionName(const char *option_name)
{
    const char *dot = strrchr(option_name, '.');
    if (dot) {
        return dot + 1;
    }
    return option_name;
}

const CONFIG_OPTION *Config_GetOptionByPath(const char *const path)
{
    if (path == nullptr) {
        return nullptr;
    }
    for (const CONFIG_OPTION *opt = Config_GetOptionMap(); opt->name != nullptr;
         opt++) {
        if (strcmp(opt->name, path) == 0
            || strcmp(Config_ResolveOptionName(opt->name), path) == 0) {
            return opt;
        }
    }
    return nullptr;
}
