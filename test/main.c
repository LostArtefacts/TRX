#include "config.h"
#include "test.h"

void test_reading_config()
{
    T1MReadConfig();

    ASSERT_INT_EQUAL(T1MConfig.disable_healing_between_levels, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_medpacks, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_magnums, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_uzis, 0);
    ASSERT_INT_EQUAL(T1MConfig.disable_shotgun, 0);
    ASSERT_INT_EQUAL(T1MConfig.enable_enemy_healthbar, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_enhanced_look, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_enhanced_ui, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_shotgun_flash, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_numeric_keys, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_tr3_sidesteps, 1);
    ASSERT_INT_EQUAL(T1MConfig.enable_cheats, 0);
    ASSERT_INT_EQUAL(T1MConfig.enable_braid, 0);
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
    ASSERT_INT_EQUAL(T1MConfig.fix_end_of_level_freeze, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_tihocan_secret_sound, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_pyramid_secret_trigger, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_hardcoded_secret_counts, 1);
    ASSERT_INT_EQUAL(T1MConfig.fix_illegal_gun_equip, 1);
    ASSERT_INT_EQUAL(T1MConfig.fov_value, 65);
    ASSERT_INT_EQUAL(T1MConfig.fov_vertical, 1);
}

int main(int argc, char* argv[])
{
    TEST_RUN(test_reading_config);
    TEST_RESULTS();
}
