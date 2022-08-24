#include "config.h"

#include "filesystem.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "json/json_base.h"
#include "json/json_parse.h"
#include "log.h"
#include "memory.h"
#include "specific/s_shell.h"
#include "util.h"

#include <string.h>

#define Q(x) #x
#define QUOTE(x) Q(x)

#define READ_PRIMITIVE(func, opt, default_value)                               \
    do {                                                                       \
        g_Config.opt =                                                         \
            func(root_obj, Config_ProcessKey(QUOTE(opt)), default_value);      \
    } while (0)
#define READ_BOOL(opt, default_value)                                          \
    READ_PRIMITIVE(json_object_get_bool, opt, default_value)
#define READ_INTEGER(opt, default_value)                                       \
    READ_PRIMITIVE(json_object_get_int, opt, default_value)
#define READ_FLOAT(opt, default_value)                                         \
    READ_PRIMITIVE(json_object_get_double, opt, default_value)

#define READ_ENUM(opt, default_value, enum_map)                                \
    do {                                                                       \
        g_Config.opt = Config_ReadEnum(                                        \
            root_obj, Config_ProcessKey(QUOTE(opt)), default_value, enum_map); \
    } while (0)

CONFIG g_Config = { 0 };

static const char *m_T1MGlobalSettingsPath = "cfg/Tomb1Main.json5";

typedef struct ENUM_MAP {
    const char *text;
    int value;
} ENUM_MAP;

const ENUM_MAP m_UIStyles[] = {
    { "ps1", UI_STYLE_PS1 },
    { "pc", UI_STYLE_PC },
    { NULL, -1 },
};

const ENUM_MAP m_BarShowingModes[] = {
    { "default", BSM_DEFAULT },
    { "flashing-or-default", BSM_FLASHING_OR_DEFAULT },
    { "flashing-only", BSM_FLASHING_ONLY },
    { "always", BSM_ALWAYS },
    { "never", BSM_NEVER },
    { "ps1", BSM_PS1 },
    { NULL, -1 },
};

const ENUM_MAP m_BarLocations[] = {
    { "top-left", BL_TOP_LEFT },
    { "top-center", BL_TOP_CENTER },
    { "top-right", BL_TOP_RIGHT },
    { "bottom-left", BL_BOTTOM_LEFT },
    { "bottom-center", BL_BOTTOM_CENTER },
    { "bottom-right", BL_BOTTOM_RIGHT },
    { NULL, -1 },
};

const ENUM_MAP m_BarColors[] = {
    { "gold", BC_GOLD },   { "blue", BC_BLUE },     { "grey", BC_GREY },
    { "red", BC_RED },     { "silver", BC_SILVER }, { "green", BC_GREEN },
    { "gold2", BC_GOLD2 }, { "blue2", BC_BLUE2 },   { "pink", BC_PINK },
    { NULL, -1 },
};

const ENUM_MAP m_ScreenshotFormats[] = {
    { "jpg", SCREENSHOT_FORMAT_JPEG },
    { "jpeg", SCREENSHOT_FORMAT_JPEG },
    { "png", SCREENSHOT_FORMAT_PNG },
    { NULL, -1 },
};

static const char *Config_ProcessKey(const char *key);
static int Config_ReadEnum(
    struct json_object_s *obj, const char *name, int8_t default_value,
    const ENUM_MAP *enum_map);

static const char *Config_ProcessKey(const char *key)
{
    return strchr(key, '.') ? strrchr(key, '.') + 1 : key;
}

static int Config_ReadEnum(
    struct json_object_s *obj, const char *name, int8_t default_value,
    const ENUM_MAP *enum_map)
{
    const char *value_str = json_object_get_string(obj, name, NULL);
    if (value_str) {
        while (enum_map->text) {
            if (!strcmp(value_str, enum_map->text)) {
                return enum_map->value;
            }
            enum_map++;
        }
    }
    return default_value;
}

bool Config_ReadFromJSON(const char *cfg_data)
{
    bool result = false;
    struct json_value_s *root;
    struct json_parse_result_s parse_result;

    root = json_parse_ex(
        cfg_data, strlen(cfg_data), json_parse_flags_allow_json5, NULL, NULL,
        &parse_result);
    if (root) {
        result = true;
    } else {
        LOG_ERROR(
            "failed to parse config file: %s in line %d, char %d",
            json_get_error_description(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no);
        // continue to supply the default values
    }

    struct json_object_s *root_obj = json_value_as_object(root);

    READ_BOOL(disable_healing_between_levels, false);
    READ_BOOL(disable_medpacks, false);
    READ_BOOL(disable_magnums, false);
    READ_BOOL(disable_uzis, false);
    READ_BOOL(disable_shotgun, false);
    READ_BOOL(enable_detailed_stats, true);
    READ_BOOL(enable_deaths_counter, true);
    READ_BOOL(enable_enemy_healthbar, true);
    READ_BOOL(enable_enhanced_look, true);
    READ_BOOL(enable_shotgun_flash, true);
    READ_BOOL(fix_shotgun_targeting, true);
    READ_BOOL(enable_cheats, false);
    READ_BOOL(enable_numeric_keys, true);
    READ_BOOL(enable_tr3_sidesteps, true);
    READ_BOOL(enable_braid, false);
    READ_BOOL(enable_compass_stats, true);
    READ_BOOL(enable_total_stats, true);
    READ_BOOL(enable_timer_in_inventory, true);
    READ_BOOL(enable_smooth_bars, true);
    READ_BOOL(enable_fade_effects, true);
    READ_BOOL(fix_tihocan_secret_sound, true);
    READ_BOOL(fix_pyramid_secret_trigger, true);
    READ_BOOL(fix_secrets_killing_music, true);
    READ_BOOL(fix_descending_glitch, false);
    READ_BOOL(fix_wall_jump_glitch, false);
    READ_BOOL(fix_bridge_collision, true);
    READ_BOOL(fix_qwop_glitch, false);
    READ_BOOL(fix_alligator_ai, true);
    READ_BOOL(change_pierre_spawn, true);
    READ_INTEGER(fov_value, 65);
    READ_INTEGER(resolution_width, -1);
    READ_INTEGER(resolution_height, -1);
    READ_BOOL(fov_vertical, true);
    READ_BOOL(disable_demo, false);
    READ_BOOL(disable_fmv, false);
    READ_BOOL(disable_cine, false);
    READ_BOOL(disable_music_in_menu, false);
    READ_BOOL(disable_music_in_inventory, false);
    READ_BOOL(enable_xbox_one_controller, false);
    READ_FLOAT(brightness, 1.0);
    READ_BOOL(enable_round_shadow, true);
    READ_BOOL(enable_3d_pickups, true);
    READ_FLOAT(rendering.anisotropy_filter, 16.0f);
    READ_BOOL(walk_to_items, false);
    READ_BOOL(disable_trex_collision, false);
    READ_INTEGER(start_lara_hitpoints, LARA_HITPOINTS);
    READ_ENUM(
        healthbar_showing_mode, BSM_FLASHING_OR_DEFAULT, m_BarShowingModes);
    READ_ENUM(airbar_showing_mode, BSM_DEFAULT, m_BarShowingModes);
    READ_ENUM(healthbar_location, BL_TOP_LEFT, m_BarLocations);
    READ_ENUM(airbar_location, BL_TOP_RIGHT, m_BarLocations);
    READ_ENUM(enemy_healthbar_location, BL_BOTTOM_LEFT, m_BarLocations);
    READ_ENUM(healthbar_color, BC_RED, m_BarColors);
    READ_ENUM(airbar_color, BC_BLUE, m_BarColors);
    READ_ENUM(enemy_healthbar_color, BC_GREY, m_BarColors);
    READ_ENUM(screenshot_format, SCREENSHOT_FORMAT_JPEG, m_ScreenshotFormats);
    READ_ENUM(ui.menu_style, UI_STYLE_PC, m_UIStyles);
    READ_INTEGER(maximum_save_slots, 25);
    READ_BOOL(revert_to_pistols, false);
    READ_BOOL(enable_enhanced_saves, true);

    CLAMP(g_Config.start_lara_hitpoints, 1, LARA_HITPOINTS);
    CLAMP(g_Config.fov_value, 30, 255);

    if (root) {
        json_value_free(root);
    }
    return result;
}

bool Config_Read(void)
{
    bool result = false;
    char *cfg_data = NULL;

    if (!File_Load(m_T1MGlobalSettingsPath, &cfg_data, NULL)) {
        LOG_ERROR("Failed to open file '%s'", m_T1MGlobalSettingsPath);
        result = Config_ReadFromJSON("");
        goto cleanup;
    }

    result = Config_ReadFromJSON(cfg_data);

    if (g_Config.resolution_width > 0) {
        g_AvailableResolutions[RESOLUTIONS_SIZE - 1].width =
            g_Config.resolution_width;
        g_AvailableResolutions[RESOLUTIONS_SIZE - 1].height =
            g_Config.resolution_height;
    } else {
        g_AvailableResolutions[RESOLUTIONS_SIZE - 1].width =
            S_Shell_GetCurrentDisplayWidth();
        g_AvailableResolutions[RESOLUTIONS_SIZE - 1].height =
            S_Shell_GetCurrentDisplayHeight();
    }

cleanup:
    Memory_FreePointer(&cfg_data);
    return result;
}
