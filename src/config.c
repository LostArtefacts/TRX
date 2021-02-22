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

static int8_t ReadBarShowingMode(json_value* root, const char* name)
{
    const char* value_str = JSONGetStringValue(root, name);
    if (!value_str) {
        return T1M_BSM_DEFAULT;
    } else if (!strcmp(value_str, "flashing")) {
        return T1M_BSM_FLASHING;
    } else if (!strcmp(value_str, "always")) {
        return T1M_BSM_ALWAYS;
    }
    return T1M_BSM_DEFAULT;
}

static int8_t
ReadBarLocationConfig(json_value* root, const char* name, int8_t default_value)
{
    const char* value_str = JSONGetStringValue(root, name);
    if (!value_str) {
        return default_value;
    } else if (!strcmp(value_str, "top-left")) {
        return T1M_BL_VTOP | T1M_BL_HLEFT;
    } else if (!strcmp(value_str, "top-center")) {
        return T1M_BL_VTOP | T1M_BL_HCENTER;
    } else if (!strcmp(value_str, "top-right")) {
        return T1M_BL_VTOP | T1M_BL_HRIGHT;
    } else if (!strcmp(value_str, "bottom-left")) {
        return T1M_BL_VBOTTOM | T1M_BL_HLEFT;
    } else if (!strcmp(value_str, "bottom-center")) {
        return T1M_BL_VBOTTOM | T1M_BL_HCENTER;
    } else if (!strcmp(value_str, "bottom-right")) {
        return T1M_BL_VBOTTOM | T1M_BL_HRIGHT;
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

    json_value* json = json_parse((const json_char*)cfg_data, cfg_size);

    READ_BOOL(disable_healing_between_levels);
    READ_BOOL(disable_medpacks);
    READ_BOOL(disable_magnums);
    READ_BOOL(disable_uzis);
    READ_BOOL(disable_shotgun);
    READ_BOOL(enable_red_healthbar);
    READ_BOOL(enable_enemy_healthbar);
    READ_BOOL(enable_enhanced_look);
    READ_BOOL(enable_enhanced_ui);
    READ_BOOL(enable_shotgun_flash);
    READ_BOOL(enable_cheats);
    READ_BOOL(enable_numeric_keys);
    READ_BOOL(fix_end_of_level_freeze);
    READ_BOOL(fix_tihocan_secret_sound);
    READ_BOOL(fix_pyramid_secret_trigger);
    READ_BOOL(fix_hardcoded_secret_counts);

    T1MConfig.healthbar_showing_mode =
        ReadBarShowingMode(json, "healthbar_showing_mode");

    T1MConfig.healthbar_location = ReadBarLocationConfig(
        json, "healthbar_location", T1M_BL_VTOP | T1M_BL_HLEFT);
    T1MConfig.airbar_location = ReadBarLocationConfig(
        json, "airbar_location", T1M_BL_VTOP | T1M_BL_HRIGHT);
    T1MConfig.enemy_healthbar_location = ReadBarLocationConfig(
        json, "enemy_healthbar_location", T1M_BL_VBOTTOM | T1M_BL_HLEFT);

    json_value_free(json);
    free(cfg_data);
    return 1;
}
