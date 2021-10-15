#include "config.h"

#include "filesystem.h"
#include "global/const.h"
#include "json.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

#define Q(x) #x
#define QUOTE(x) Q(x)

#define READ_PRIMITIVE(func, opt, default_value)                               \
    do {                                                                       \
        T1MConfig.opt = func(root_obj, QUOTE(opt), default_value);             \
    } while (0)
#define READ_BOOL(opt, default_value)                                          \
    READ_PRIMITIVE(json_object_get_bool, opt, default_value)
#define READ_INTEGER(opt, default_value)                                       \
    READ_PRIMITIVE(json_object_get_number_int, opt, default_value)

#define READ_CUSTOM(func, opt, default_value)                                  \
    do {                                                                       \
        T1MConfig.opt = func(root_obj, QUOTE(opt), default_value);             \
    } while (0)
#define READ_BAR_SHOWING_MODE(opt, default_value)                              \
    READ_CUSTOM(ReadBarShowingMode, opt, default_value)
#define READ_BAR_LOCATION(opt, default_value)                                  \
    READ_CUSTOM(ReadBarLocation, opt, default_value)
#define READ_BAR_COLOR(opt, default_value)                                     \
    READ_CUSTOM(ReadBarColor, opt, default_value)

T1MConfigStruct T1MConfig = { 0 };

static int8_t ReadBarShowingMode(
    struct json_object_s *obj, const char *name, int8_t default_value)
{
    const char *value_str = json_object_get_string(obj, name, NULL);
    if (value_str) {
        if (!strcmp(value_str, "flashing-or-default")) {
            return T1M_BSM_FLASHING_OR_DEFAULT;
        } else if (!strcmp(value_str, "flashing-only")) {
            return T1M_BSM_FLASHING_ONLY;
        } else if (!strcmp(value_str, "always")) {
            return T1M_BSM_ALWAYS;
        } else if (!strcmp(value_str, "never")) {
            return T1M_BSM_NEVER;
        }
    }
    return default_value;
}

static int8_t ReadBarLocation(
    struct json_object_s *obj, const char *name, int8_t default_value)
{
    const char *value_str = json_object_get_string(obj, name, NULL);
    if (value_str) {
        if (!strcmp(value_str, "top-left")) {
            return T1M_BL_TOP_LEFT;
        } else if (!strcmp(value_str, "top-center")) {
            return T1M_BL_TOP_CENTER;
        } else if (!strcmp(value_str, "top-right")) {
            return T1M_BL_TOP_RIGHT;
        } else if (!strcmp(value_str, "bottom-left")) {
            return T1M_BL_BOTTOM_LEFT;
        } else if (!strcmp(value_str, "bottom-center")) {
            return T1M_BL_BOTTOM_CENTER;
        } else if (!strcmp(value_str, "bottom-right")) {
            return T1M_BL_BOTTOM_RIGHT;
        }
    }
    return default_value;
}

static int8_t
ReadBarColor(struct json_object_s *obj, const char *name, int8_t default_value)
{
    const char *value_str = json_object_get_string(obj, name, NULL);
    if (value_str) {
        if (!strcmp(value_str, "gold")) {
            return T1M_BC_GOLD;
        } else if (!strcmp(value_str, "blue")) {
            return T1M_BC_BLUE;
        } else if (!strcmp(value_str, "grey")) {
            return T1M_BC_GREY;
        } else if (!strcmp(value_str, "red")) {
            return T1M_BC_RED;
        } else if (!strcmp(value_str, "silver")) {
            return T1M_BC_SILVER;
        } else if (!strcmp(value_str, "green")) {
            return T1M_BC_GREEN;
        } else if (!strcmp(value_str, "gold2")) {
            return T1M_BC_GOLD2;
        } else if (!strcmp(value_str, "blue2")) {
            return T1M_BC_BLUE2;
        } else if (!strcmp(value_str, "pink")) {
            return T1M_BC_PINK;
        }
    }
    return default_value;
}

int8_t T1MReadConfigFromJson(const char *cfg_data)
{
    int8_t result = 0;
    struct json_value_s *root;
    struct json_parse_result_s parse_result;

    root = json_parse_ex(
        cfg_data, strlen(cfg_data), json_parse_flags_allow_json5, NULL, NULL,
        &parse_result);
    if (!root) {
        LOG_ERROR(
            "failed to parse config file: %s in line %d, char %d",
            json_get_error_description(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no);
        // continue to supply the default values
    } else {
        result = 1;
    }

    struct json_object_s *root_obj = json_value_as_object(root);

    READ_BOOL(disable_healing_between_levels, 0);
    READ_BOOL(disable_medpacks, 0);
    READ_BOOL(disable_magnums, 0);
    READ_BOOL(disable_uzis, 0);
    READ_BOOL(disable_shotgun, 0);
    READ_BOOL(enable_enemy_healthbar, 1);
    READ_BOOL(enable_enhanced_look, 1);
    READ_BOOL(enable_shotgun_flash, 1);
    READ_BOOL(enable_cheats, 0);
    READ_BOOL(enable_numeric_keys, 1);
    READ_BOOL(enable_tr3_sidesteps, 1);
    READ_BOOL(enable_braid, 0);
    READ_BOOL(enable_compass_stats, 1);
    READ_BOOL(enable_timer_in_inventory, 1);
    READ_BOOL(enable_smooth_bars, 1);
    READ_BOOL(fix_tihocan_secret_sound, 1);
    READ_BOOL(fix_pyramid_secret_trigger, 1);
    READ_BOOL(fix_secrets_killing_music, 1);
    READ_INTEGER(fov_value, 65);
    READ_BOOL(fov_vertical, 1);
    READ_BOOL(disable_demo, 0);
    READ_BOOL(disable_fmv, 0);
    READ_BOOL(disable_cine, 0);

    READ_BAR_SHOWING_MODE(healthbar_showing_mode, T1M_BSM_FLASHING_OR_DEFAULT);
    READ_BAR_SHOWING_MODE(airbar_showing_mode, T1M_BSM_DEFAULT);
    READ_BAR_LOCATION(healthbar_location, T1M_BL_TOP_LEFT);
    READ_BAR_LOCATION(airbar_location, T1M_BL_TOP_RIGHT);
    READ_BAR_LOCATION(enemy_healthbar_location, T1M_BL_BOTTOM_LEFT);
    READ_BAR_COLOR(healthbar_color, T1M_BC_RED);
    READ_BAR_COLOR(airbar_color, T1M_BC_BLUE);
    READ_BAR_COLOR(enemy_healthbar_color, T1M_BC_GREY);

    CLAMP(T1MConfig.fov_value, 30, 255);

    if (root) {
        json_value_free(root);
    }
    return result;
}

int8_t T1MReadConfig()
{
    int8_t result = 0;
    MYFILE *fp = NULL;
    char *cfg_data = NULL;

    fp = FileOpen("cfg/Tomb1Main.json5", FILE_OPEN_READ);
    if (!fp) {
        result = T1MReadConfigFromJson("");
        goto cleanup;
    }

    size_t cfg_data_size = FileSize(fp);

    cfg_data = malloc(cfg_data_size + 1);
    if (!cfg_data) {
        result = T1MReadConfigFromJson("");
        goto cleanup;
    }
    FileRead(cfg_data, 1, cfg_data_size, fp);
    cfg_data[cfg_data_size] = '\0';
    FileClose(fp);
    fp = NULL;

    result = T1MReadConfigFromJson(cfg_data);

cleanup:
    if (fp) {
        FileClose(fp);
    }
    if (cfg_data) {
        free(cfg_data);
    }
    return result;
}
