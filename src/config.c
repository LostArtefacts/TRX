#include <stdio.h>
#include <string.h>
#include "json_utils.h"
#include "config.h"

#define Q(x) #x
#define QUOTE(x) Q(x)
#define READ_BOOL(OPT)                                                         \
    do {                                                                       \
        T1MConfig.OPT = JSONGetBooleanValue(json, QUOTE(OPT));                 \
    } while (0)

static int8_t ReadBarShowingMode(struct json_value_s* root, const char* name)
{
    const char* value_str = JSONGetStringValue(root, name);
    if (!value_str) {
        return T1M_BSM_DEFAULT;
    } else if (!strcmp(value_str, "flashing")) {
        return T1M_BSM_FLASHING;
    } else if (!strcmp(value_str, "always")) {
        return T1M_BSM_ALWAYS;
    } else if (!strcmp(value_str, "never")) {
        return T1M_BSM_NEVER;
    }
    return T1M_BSM_DEFAULT;
}

static int8_t ReadBarColorConfig(
    struct json_value_s* root, const char* name, int8_t default_value)
{
    const char* value_str = JSONGetStringValue(root, name);
    if (!value_str) {
        return default_value;
    } else if (!strcmp(value_str, "gold")) {
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
    return default_value;
}

static int8_t ReadBarLocationConfig(
    struct json_value_s* root, const char* name, int8_t default_value)
{
    const char* value_str = JSONGetStringValue(root, name);
    if (!value_str) {
        return default_value;
    } else if (!strcmp(value_str, "top-left")) {
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
    return default_value;
}

int T1MReadConfig()
{
    FILE* fp = fopen("Tomb1Main.json", "rb");
    if (!fp) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    int cfg_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* cfg_data = malloc(cfg_size);
    if (!cfg_data) {
        fclose(fp);
        return 0;
    }
    fread(cfg_data, 1, cfg_size, fp);
    fclose(fp);

    struct json_value_s* json = json_parse_ex(
        cfg_data, cfg_size, json_parse_flags_allow_json5, NULL, NULL, NULL);

    READ_BOOL(disable_healing_between_levels);
    READ_BOOL(disable_medpacks);
    READ_BOOL(disable_magnums);
    READ_BOOL(disable_uzis);
    READ_BOOL(disable_shotgun);
    READ_BOOL(enable_enemy_healthbar);
    READ_BOOL(enable_enhanced_look);
    READ_BOOL(enable_enhanced_ui);
    READ_BOOL(enable_shotgun_flash);
    READ_BOOL(enable_cheats);
    READ_BOOL(enable_numeric_keys);
    READ_BOOL(enable_tr3_sidesteps);
    READ_BOOL(enable_braid);
    READ_BOOL(fix_key_triggers);
    READ_BOOL(fix_end_of_level_freeze);
    READ_BOOL(fix_tihocan_secret_sound);
    READ_BOOL(fix_pyramid_secret_trigger);
    READ_BOOL(fix_hardcoded_secret_counts);
    READ_BOOL(fix_illegal_gun_equip);

    T1MConfig.healthbar_showing_mode =
        ReadBarShowingMode(json, "healthbar_showing_mode");
    T1MConfig.airbar_showing_mode =
        ReadBarShowingMode(json, "airbar_showing_mode");

    T1MConfig.healthbar_location =
        ReadBarLocationConfig(json, "healthbar_location", T1M_BL_TOP_LEFT);
    T1MConfig.airbar_location =
        ReadBarLocationConfig(json, "airbar_location", T1M_BL_TOP_RIGHT);
    T1MConfig.enemy_healthbar_location = ReadBarLocationConfig(
        json, "enemy_healthbar_location", T1M_BL_BOTTOM_LEFT);

    T1MConfig.healthbar_color =
        ReadBarColorConfig(json, "healthbar_color", T1M_BC_RED);
    T1MConfig.airbar_color =
        ReadBarColorConfig(json, "airbar_color", T1M_BC_BLUE);
    T1MConfig.enemy_healthbar_color =
        ReadBarColorConfig(json, "enemy_healthbar_color", T1M_BC_GREY);

    free(json);
    free(cfg_data);
    return 1;
}
