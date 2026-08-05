#include <trx/config/option.h>
#include <trx/config/priv.h>
#include <trx/config/registry.h>
#include <trx/config/types.h>
#include <trx/config/vars.h>
#include <trx/core/colors.h>
#include <trx/core/enum_map.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/clock.h>
#include <trx/game/input.h>
#include <trx/game/lara/const.h>
#include <trx/version.h>

// What each game's map*.def says. These are descriptions, not options: an
// option is made from one as the game starts, through the same call anything
// else would use.
//
// A `_RANGE` variant says what the option accepts. The bounds belong to the
// option from here on, so the settings row that walks it and the pass that
// holds a hand-edited file to it read the same numbers. They are written in the
// units the value is stored in, which for a percentage is 0..1 rather than the
// 0..100 the dialogs show.

#define M_BOUNDS(min_, max_) &(const CONFIG_OPTION_BOUNDS) { min_, max_ }

#define X_CFG_BOOL(target_, default_value_)                                    \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_BOOL, .as_bool = default_value_ },        \
      .mirror = &g_ConfigStorage.target_ },

#define X_CFG_INT32(target_, default_value_)                                   \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_S32, .as_int = default_value_ },          \
      .mirror = &g_ConfigStorage.target_ },

#define X_CFG_INT32_RANGE(target_, default_value_, min_, max_)                 \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_S32, .as_int = default_value_ },          \
      .mirror = &g_ConfigStorage.target_,                                      \
      .bounds = M_BOUNDS(min_, max_) },

#define X_CFG_FLOAT(target_, default_value_)                                   \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_FLOAT, .as_num = default_value_ },        \
      .mirror = &g_ConfigStorage.target_ },

#define X_CFG_FLOAT_RANGE(target_, default_value_, min_, max_)                 \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_FLOAT, .as_num = default_value_ },        \
      .mirror = &g_ConfigStorage.target_,                                      \
      .bounds = M_BOUNDS(min_, max_) },

#define X_CFG_FLOAT_PERCENT(target_, default_value_)                           \
    X_CFG_FLOAT_PERCENT_RANGE(target_, default_value_, 0.0, 1.0)

#define X_CFG_FLOAT_PERCENT_RANGE(target_, default_value_, min_, max_)         \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_FLOAT, .as_num = default_value_ },        \
      .mirror = &g_ConfigStorage.target_,                                      \
      .percent = true,                                                         \
      .bounds = M_BOUNDS(min_, max_) },

#define X_CFG_DOUBLE(target_, default_value_)                                  \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_DOUBLE, .as_num = default_value_ },       \
      .mirror = &g_ConfigStorage.target_ },

#define X_CFG_ENUM(target_, default_value_, enum_map_)                         \
    X_CFG_ENUM_EX(QUOTE(target_), target_, default_value_, enum_map_)

#define X_CFG_ENUM_EX(name_, target_, default_value_, enum_map_)               \
    { .name = name_,                                                           \
      .default_value = { .type = TVT_ENUM, .as_int = default_value_ },         \
      .mirror = &g_ConfigStorage.target_,                                      \
      .enum_map = ENUM_MAP_NAME(enum_map_) },

#define X_CFG_ENUM_RANGE(target_, default_value_, enum_map_, min_, max_)       \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_ENUM, .as_int = default_value_ },         \
      .mirror = &g_ConfigStorage.target_,                                      \
      .enum_map = ENUM_MAP_NAME(enum_map_),                                    \
      .bounds = M_BOUNDS(min_, max_) },

#define X_CFG_RGB888(target_, default_r, default_g, default_b)                 \
    { .name = QUOTE(target_),                                                  \
      .default_value = { .type = TVT_RGB_888,                                  \
                         .as_rgb = { default_r, default_g, default_b } },      \
      .mirror = &g_ConfigStorage.target_ },

#define X_CFG_STRING(target_, default_value_)                                  \
    X_CFG_STRING_EX(QUOTE(target_), target_, default_value_)

#define X_CFG_STRING_EX(name_, target_, default_value_)                        \
    { .name = name_,                                                           \
      .default_value = { .type = TVT_STRING, .as_str = default_value_ },       \
      .mirror = &g_ConfigStorage.target_ },

#define X_CFG_DYNAMIC_ENUM(target_, default_value_)                            \
    X_CFG_DYNAMIC_ENUM_EX(QUOTE(target_), target_, default_value_)

#define X_CFG_DYNAMIC_ENUM_EX(name_, target_, default_value_)                  \
    { .name = name_,                                                           \
      .default_value = { .type = TVT_DYNAMIC_ENUM, .as_str = default_value_ }, \
      .mirror = &g_ConfigStorage.target_ },

static const CONFIG_OPTION_DESC *m_Descs[TR_VERSION_COUNT] = {
    [0] =
        (const CONFIG_OPTION_DESC[]) {
#include <trx/config/map_tr1.def>
            {}, // sentinel
        },
    [1] =
        (const CONFIG_OPTION_DESC[]) {
#include <trx/config/map_tr2.def>
            {}, // sentinel
        },
    [2] =
        (const CONFIG_OPTION_DESC[]) {
#include <trx/config/map_tr3.def>
            {}, // sentinel
        },
    [3] =
        (const CONFIG_OPTION_DESC[]) {
#include <trx/config/map_tr4.def>
            {}, // sentinel
        },
};

#undef X_CFG_BOOL
#undef X_CFG_INT32
#undef X_CFG_INT32_RANGE
#undef X_CFG_FLOAT
#undef X_CFG_FLOAT_RANGE
#undef X_CFG_FLOAT_PERCENT
#undef X_CFG_FLOAT_PERCENT_RANGE
#undef X_CFG_DOUBLE
#undef X_CFG_ENUM
#undef X_CFG_ENUM_EX
#undef X_CFG_ENUM_RANGE
#undef X_CFG_RGB888
#undef X_CFG_STRING
#undef X_CFG_STRING_EX
#undef X_CFG_DYNAMIC_ENUM
#undef X_CFG_DYNAMIC_ENUM_EX
#undef M_BOUNDS

void Config_RegisterBuiltInOptions(void)
{
    ASSERT(g_TRVersion >= 1 && g_TRVersion <= TR_VERSION_COUNT);
    // A different game names a different set of settings, so nothing the last
    // one registered carries over.
    Config_DropAllOptions();
    for (const CONFIG_OPTION_DESC *desc = m_Descs[g_TRVersion - 1];
         desc->name != nullptr; desc++) {
        const CONFIG_OPTION *const option = Config_Register(desc);
        // Two declarations of one setting is a mistake in map*.def, not
        // something to resolve at runtime: whichever lost would silently take
        // the other's default, and a game's own default is exactly what a
        // second declaration is written to say. A script declaring one is a
        // different matter - it is told no, and says so in Lua.
        ASSERT(option != nullptr);
    }
}
