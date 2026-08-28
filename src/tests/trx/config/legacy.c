// The migrations that carry an older release's settings file forward.
//
// Each reads a key that release wrote and applies it to the option that
// replaced it, but only while that option is not in the file: one that is
// there is where the player's answer went, and reading the old key over it
// would put back what they changed away from.

#include <harness/harness.h>

#include <trx/config/common.h>
#include <trx/config/legacy.h>
#include <trx/config/option.h>
#include <trx/config/registry.h>
#include <trx/config/vars.h>
#include <trx/version.h>

#include <string.h>

// The options a migration reaches for by name rather than through g_Config.
// Standing the registry up would bring the settings file with it, so these
// answer for the three options the migrations name and record what was put
// back.
static CONFIG_OPTION m_LightingCurveOption;
static CONFIG_OPTION m_BrightnessOption;
static CONFIG_OPTION m_GammaOption;
static bool m_HasLightingCurve;
static bool m_BrightnessRestored;
static bool m_GammaRestored;

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

CONFIG_OPTION *Config_FindOption(const char *const path)
{
    if (m_HasLightingCurve && strcmp(path, "rendering.lighting_curve") == 0) {
        return &m_LightingCurveOption;
    }
    return nullptr;
}

CONFIG_OPTION *Config_FindOptionByMirror(const void *const mirror)
{
    if (mirror == &g_Config.visuals.game_brightness) {
        return &m_BrightnessOption;
    }
    if (mirror == &g_Config.visuals.gamma) {
        return &m_GammaOption;
    }
    return nullptr;
}

bool Config_Option_RestoreDefault(CONFIG_OPTION *const option, const bool force)
{
    m_BrightnessRestored |= option == &m_BrightnessOption;
    m_GammaRestored |= option == &m_GammaOption;
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
    g_ConfigStorage.visuals.breeze_mode = BREEZE_MODE_TR3;
    M_Load("{\"enable_breeze\":false}");
    CHECK_EQ_INT(g_Config.visuals.breeze_mode, BREEZE_MODE_OFF);
}

TEST(the_old_dithering_choice_is_carried_to_the_software_renderer_mode)
{
    g_ConfigStorage.rendering.dither_mode = DITHER_MODE_DISABLED;
    M_Load("{\"enable_dithering\":true}");
    CHECK_EQ_INT(g_Config.rendering.dither_mode, DITHER_MODE_SOFTWARE_RENDERER);

    g_ConfigStorage.rendering.dither_mode = DITHER_MODE_PS1;
    M_Load("{\"enable_dithering\":false}");
    CHECK_EQ_INT(g_Config.rendering.dither_mode, DITHER_MODE_DISABLED);

    g_ConfigStorage.rendering.dither_mode = DITHER_MODE_PS1;
    M_Load("{\"enable_dithering\":false,\"dither_mode\":\"ps1\"}");
    CHECK_EQ_INT(g_Config.rendering.dither_mode, DITHER_MODE_PS1);
}

TEST(vertex_snap_booleans_migrate_to_the_fixed_resolution)
{
    g_ConfigStorage.rendering.vertex_snap_mode = VERTEX_SNAP_MODE_DISABLED;
    M_Load("{\"enable_vertex_snap\":true}");
    CHECK_EQ_INT(g_Config.rendering.vertex_snap_mode, VERTEX_SNAP_MODE_320X240);
}

TEST(vertex_snap_at_upscale_migrates_to_the_upscale_resolution)
{
    g_ConfigStorage.rendering.vertex_snap_mode = VERTEX_SNAP_MODE_DISABLED;
    M_Load(
        "{\"enable_vertex_snap\":true,"
        "\"enable_vertex_snap_at_upscale\":true}");
    CHECK_EQ_INT(
        g_Config.rendering.vertex_snap_mode, VERTEX_SNAP_MODE_UPSCALE_RES);
}

TEST(a_disabled_vertex_snap_stays_disabled_during_migration)
{
    g_ConfigStorage.rendering.vertex_snap_mode = VERTEX_SNAP_MODE_320X240;
    M_Load(
        "{\"enable_vertex_snap\":false,"
        "\"enable_vertex_snap_at_upscale\":true}");
    CHECK_EQ_INT(
        g_Config.rendering.vertex_snap_mode, VERTEX_SNAP_MODE_DISABLED);
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
    CHECK_EQ_INT(g_Config.config_version, 2);

    // A file with no version at all is one nothing has written yet.
    g_ConfigStorage.config_version = -1;
    M_Load("{}");
    CHECK_EQ_INT(g_Config.config_version, -1);
}

TEST(a_file_that_predates_the_lighting_curve_gives_up_its_brightness)
{
    m_HasLightingCurve = true;
    m_BrightnessRestored = false;
    m_GammaRestored = false;
    g_ConfigStorage.rendering.lighting_curve = LIGHTING_CURVE_OVERBRIGHT;
    M_Load("{\"game_brightness\":1.5}");
    CHECK_EQ_INT(g_Config.rendering.lighting_curve, LIGHTING_CURVE_SATURATE);
    CHECK(m_BrightnessRestored);
    CHECK(m_GammaRestored);
}

TEST(a_file_that_carries_the_lighting_curve_keeps_its_brightness)
{
    m_HasLightingCurve = true;
    m_BrightnessRestored = false;
    m_GammaRestored = false;
    g_ConfigStorage.rendering.lighting_curve = LIGHTING_CURVE_FLAT;
    M_Load("{\"game_brightness\":1.5,\"lighting_curve\":\"flat\"}");
    CHECK_EQ_INT(g_Config.rendering.lighting_curve, LIGHTING_CURVE_FLAT);
    CHECK(!m_BrightnessRestored);
    CHECK(!m_GammaRestored);
}

TEST(a_game_without_the_lighting_curve_keeps_its_brightness)
{
    m_HasLightingCurve = false;
    m_BrightnessRestored = false;
    m_GammaRestored = false;
    M_Load("{\"game_brightness\":1.5}");
    CHECK(!m_BrightnessRestored);
    CHECK(!m_GammaRestored);
}

TEST(a_key_a_migration_reads_is_one_the_writer_drops)
{
    // The answer stands on the migrations this build has, not on what any file
    // held or on a load having happened first.
    CHECK(ConfigLegacy_IsKey("enable_target_change"));
    CHECK(ConfigLegacy_IsKey("enable_breeze"));
    CHECK(ConfigLegacy_IsKey("enable_save_crystals"));
    CHECK(ConfigLegacy_IsKey("enable_game_modes"));
    CHECK(ConfigLegacy_IsKey("enable_dithering"));
    CHECK(ConfigLegacy_IsKey("enable_vertex_snap"));
    CHECK(ConfigLegacy_IsKey("enable_vertex_snap_at_upscale"));
    CHECK(ConfigLegacy_IsKey("game_brightness"));
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

TEST(braid_booleans_migrate_to_mode)
{
    g_ConfigStorage.visuals.braid_status = BRAID_STATUS_ON;
    M_Load("{\"enable_braid\":false}");
    CHECK_EQ_INT(g_Config.visuals.braid_status, BRAID_STATUS_OFF);
}
