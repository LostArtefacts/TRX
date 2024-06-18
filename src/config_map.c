#include "config_map.h"

#include "config.h"
#include "gfx/common.h"
#include "global/const.h"
#include "global/enum_str.h"
#include "global/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CFG_BOOL(target_, default_value_)                                      \
    { .name = QUOTE(target_),                                                  \
      .type = COT_BOOL,                                                        \
      .target = &g_Config.target_,                                             \
      .default_value = &(bool) { default_value_ },                             \
      .param = NULL },

#define CFG_INT32(target_, default_value_)                                     \
    { .name = QUOTE(target_),                                                  \
      .type = COT_INT32,                                                       \
      .target = &g_Config.target_,                                             \
      .default_value = &(int32_t) { default_value_ },                          \
      .param = NULL },

#define CFG_FLOAT(target_, default_value_)                                     \
    { .name = QUOTE(target_),                                                  \
      .type = COT_FLOAT,                                                       \
      .target = &g_Config.target_,                                             \
      .default_value = &(float) { default_value_ },                            \
      .param = NULL },

#define CFG_DOUBLE(target_, default_value_)                                    \
    { .name = QUOTE(target_),                                                  \
      .type = COT_DOUBLE,                                                      \
      .target = &g_Config.target_,                                             \
      .default_value = &(double) { default_value_ },                           \
      .param = NULL },

#define CFG_ENUM(target_, default_value_, enum_map)                            \
    { .name = QUOTE(target_),                                                  \
      .type = COT_ENUM,                                                        \
      .target = &g_Config.target_,                                             \
      .default_value = &(int32_t) { default_value_ },                          \
      .param = ENUM_STR_MAP(enum_map) },

const CONFIG_OPTION g_ConfigOptionMap[] = {
#include "config_map.def"
    // guard
    { 0 },
};
