#include "test_config.h"

#include "config.h"
#include "filesystem.h"
#include "test.h"

#include <stdlib.h>

void test_empty_config()
{
    T1MReadConfigFromJson("{}");

    ASSERT_INT_EQUAL(T1MConfig.disable_healing_between_levels, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_medpacks, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_magnums, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_uzis, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_shotgun, 0);
    ASSERT_INT_EQUAL(T1MConfig.enable_enemy_healthbar, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_enhanced_look, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_shotgun_flash, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_numeric_keys, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_tr3_sidesteps, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_cheats, 0);
    ASSERT_INT_EQUAL(T1MConfig.enable_braid, 0);
    ASSERT_INT_EQUAL(T1MConfig.enable_compass_stats, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_timer_in_inventory, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_smooth_bars, 1);
    ASSERT_INT_EQUAL(
        T1MConfig.healthbar_showing_mode, T1M_BSM_FLASHING_OR_DEFAULT);
    ASSERT_INT_EQUAL(T1MConfig.healthbar_location, T1M_BL_TOP_LEFT);
    ASSERT_INT_EQUAL(T1MConfig.healthbar_color, T1M_BC_RED);
    ASSERT_INT_EQUAL(T1MConfig.airbar_showing_mode, T1M_BSM_DEFAULT);
    ASSERT_INT_EQUAL(T1MConfig.airbar_location, T1M_BL_TOP_RIGHT);
    ASSERT_INT_EQUAL(T1MConfig.airbar_color, T1M_BC_BLUE);
    ASSERT_INT_EQUAL(T1MConfig.enemy_healthbar_location, T1M_BL_BOTTOM_LEFT);
    ASSERT_INT_EQUAL(T1MConfig.enemy_healthbar_color, T1M_BC_GREY);
    ASSERT_INT_EQUAL(T1MConfig.fix_key_triggers, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_tihocan_secret_sound, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_pyramid_secret_trigger, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_illegal_gun_equip, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_secrets_killing_music, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_creature_dist_calc, 1);
    ASSERT_INT_EQUAL(T1MConfig.fov_value, 65);
    ASSERT_INT_EQUAL(T1MConfig.fov_vertical, 1);
    ASSERT_INT_EQUAL(T1MConfig.disable_demo, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_fmv, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_cine, 0);
}

void test_config_override()
{
    MYFILE *fp = FileOpen("cfg/Tomb1Main.json5", FILE_OPEN_READ);
    ASSERT_OK(!!fp);

    int cfg_size = FileSize(fp);
    char *cfg_data = malloc(cfg_size + 1);
    ASSERT_OK(!!cfg_data);

    FileRead(cfg_data, 1, cfg_size, fp);
    cfg_data[cfg_size] = '\0';
    FileClose(fp);

    char *tmp = strstr(cfg_data, "enable_cheats\": false,");
    ASSERT_OK(!!tmp);
    tmp = strstr(tmp, "false,");
    ASSERT_OK(!!tmp);
    memcpy(tmp, "true, ", 6);

    T1MReadConfigFromJson("{}");
    ASSERT_INT_EQUAL(T1MConfig.enable_cheats, 0);

    T1MReadConfigFromJson(cfg_data);
    ASSERT_INT_EQUAL(T1MConfig.enable_cheats, 1);

    free(cfg_data);
}
