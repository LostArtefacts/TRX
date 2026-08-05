// The migrations that carry an older release's settings file forward.
//
// Each reads a key that release wrote and applies it to the option that
// replaced it, but only while that option is not in the file: one that is
// there is where the player's answer went, and reading the old key over it
// would put back what they changed away from.

#include <harness/harness.h>

#include <trx/config/common.h>
#include <trx/config/legacy.h>
#include <trx/config/vars.h>
#include <trx/version.h>

#include <string.h>

// A migration writes through the option that owns the setting. Standing one up
// would mean the registry and the settings file behind it, and neither is what
// a migration is being tested for, so this puts the value where the option
// would have put it and nothing else happens.
static void M_Load(const char *const text)
{
    JSON_VALUE *const root = JSON_Parse(text, strlen(text));
    ConfigLegacy_Load(JSON_ValueAsObject(root));
    JSON_ValueFree(root);
}

bool Config_SetValue(const void *const mirror, const TRX_VALUE value)
{
    // Every setting a migration writes is int32 storage: a mode the file spells
    // as a name, or the version it was last written by.
    *(int32_t *)mirror = value.as_int;
    return true;
}

TEST(a_legacy_key_is_read_while_the_option_that_replaced_it_is_absent)
{
    g_ConfigStorage.gameplay.target_change_mode = TARGET_CHANGE_MODE_OFF;
    M_Load("{\"enable_target_change\":true}");
    CHECK_EQ_INT(
        g_Config.gameplay.target_change_mode, TARGET_CHANGE_MODE_ENHANCED);
}

TEST(a_legacy_key_gives_way_to_the_option_that_replaced_it)
{
    g_ConfigStorage.gameplay.target_change_mode = TARGET_CHANGE_MODE_OFF;
    M_Load("{\"enable_target_change\":true,\"target_change_mode\":\"off\"}");
    CHECK_EQ_INT(g_Config.gameplay.target_change_mode, TARGET_CHANGE_MODE_OFF);
}

TEST(a_migration_that_only_carries_an_opt_in_leaves_the_rest_alone)
{
    // The per-game default covers a player who never asked for anything, so
    // only the ones who did are carried over.
    g_ConfigStorage.gameplay.save_crystal_mode = SAVE_CRYSTAL_HEAL;
    M_Load("{\"enable_save_crystals\":false}");
    CHECK_EQ_INT(g_Config.gameplay.save_crystal_mode, SAVE_CRYSTAL_HEAL);

    M_Load("{\"enable_save_crystals\":true}");
    CHECK_EQ_INT(g_Config.gameplay.save_crystal_mode, SAVE_CRYSTAL_SAVE);
}

TEST(a_migration_that_reads_the_game_answers_for_the_game_it_is_run_for)
{
    g_TRVersion = 3;
    M_Load("{\"enable_breeze\":true}");
    CHECK_EQ_INT(g_Config.visuals.breeze_mode, BREEZE_MODE_TR3);

    g_TRVersion = 2;
    M_Load("{\"enable_breeze\":true}");
    CHECK_EQ_INT(g_Config.visuals.breeze_mode, BREEZE_MODE_TR2);
}

TEST(a_file_the_migrations_have_nothing_to_say_about_is_left_alone)
{
    g_ConfigStorage.visuals.breeze_mode = BREEZE_MODE_TR2;
    g_ConfigStorage.gameplay.save_crystal_mode = SAVE_CRYSTAL_HEAL;
    M_Load("{\"breeze_mode\":\"tr3\",\"save_crystal_mode\":\"save\"}");
    CHECK_EQ_INT(g_Config.visuals.breeze_mode, BREEZE_MODE_TR2);
    CHECK_EQ_INT(g_Config.gameplay.save_crystal_mode, SAVE_CRYSTAL_HEAL);
}

TEST(the_version_the_file_was_last_written_by_is_brought_up_to_date)
{
    g_ConfigStorage.config_version = 0;
    M_Load("{}");
    CHECK_EQ_INT(g_Config.config_version, 1);

    // A file with no version at all is one nothing has written yet.
    g_ConfigStorage.config_version = -1;
    M_Load("{}");
    CHECK_EQ_INT(g_Config.config_version, -1);
}

TEST(a_key_a_migration_reads_is_one_the_writer_drops)
{
    // The answer stands on the migrations this build has, not on what any file
    // held or on a load having happened first.
    CHECK(ConfigLegacy_IsKey("enable_target_change"));
    CHECK(ConfigLegacy_IsKey("enable_breeze"));
    CHECK(ConfigLegacy_IsKey("enable_save_crystals"));
    CHECK(ConfigLegacy_IsKey("enable_game_modes"));
}

TEST(an_option_a_game_still_writes_is_not_a_legacy_key)
{
    // config/file.c asks this about every key it did not write, and a mod's
    // declared option is the case it must answer no for.
    CHECK(!ConfigLegacy_IsKey("water_color_mode"));
    CHECK(!ConfigLegacy_IsKey("target_change_mode"));
    CHECK(!ConfigLegacy_IsKey("bar_look"));
    CHECK(!ConfigLegacy_IsKey(nullptr));
}
