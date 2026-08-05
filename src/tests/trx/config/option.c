// The hold stack, against a config of three options and nothing else. No game,
// no settings file, no registry: an option is made here from the same
// description map*.def would give.
//
// What is under test is that the player's value survives. A level, a script or
// a demo holds a setting away from it, and when the hold is lifted, the
// player's own value is what comes back - never the default, and never the
// value some other hold applied. And while a hold is on, that is still the
// value the settings file would carry.

#include <harness/harness.h>

#include <trx/config/option.h>
#include <trx/config/priv.h>
#include <trx/core/utils.h>

#include <string.h>

// The player's config: what they chose, in the three shapes that behave
// differently - a bool, a number, and a string, which is the one that owns
// memory.
static bool m_MusicOn;
static int32_t m_Fov;
static char *m_WaterColor;

static CONFIG_OPTION m_Options[3];

static const CONFIG_OPTION_DESC m_Descs[] = {
    { .name = "audio.music",
      .default_value = { .type = TVT_BOOL, .as_bool = true },
      .mirror = &m_MusicOn },
    { .name = "visuals.fov",
      .default_value = { .type = TVT_S32, .as_int = 80 },
      .mirror = &m_Fov },
    { .name = "visuals.water_color",
      .default_value = { .type = TVT_STRING, .as_str = "ffffff" },
      .mirror = &m_WaterColor },
};

static CONFIG_OPTION *M_Option(const char *const name)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Options); i++) {
        if (strcmp(m_Options[i].name, name) == 0) {
            return &m_Options[i];
        }
    }
    return nullptr;
}

// The player's own values, which are not the defaults: a hold putting back the
// default rather than what the player chose is the failure this is looking for.
static void M_SetUp(void)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Options); i++) {
        if (m_Options[i].name != nullptr) {
            Config_Option_Free(&m_Options[i]);
        }
        m_Options[i] = (CONFIG_OPTION) {};
        Config_Option_Init(&m_Options[i], &m_Descs[i]);
    }
    Config_Option_Write(
        M_Option("visuals.fov"),
        &(TRX_VALUE) { .type = TVT_S32, .as_int = 65 });
    Config_Option_Write(
        M_Option("visuals.water_color"),
        &(TRX_VALUE) { .type = TVT_STRING, .as_str = "ff0000" });
}

static void M_Hold(
    CONFIG_OPTION *const option, const TRX_VALUE value,
    const CONFIG_HOLD_SOURCE source)
{
    CHECK(Config_Option_PushHold(option, &value, source));
}

// Nothing here is listening for what moved; the report is the module's job, and
// the module is not standing.
void Config_ReportChange(const CONFIG_OPTION *const option, const bool persist)
{
}

TEST(a_hold_takes_the_option_and_releasing_it_gives_it_back)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 }, CONFIG_HOLD_SCRIPT);
    CHECK_EQ_INT(m_Fov, 90);
    CHECK(Config_Option_IsHeld(fov));

    CHECK(Config_Option_PopHold(fov));
    CHECK_EQ_INT(m_Fov, 65);
    CHECK(!Config_Option_IsHeld(fov));
}

TEST(a_held_option_still_saves_the_players_own_value)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 },
        CONFIG_HOLD_GAME_FLOW);
    CHECK_EQ_INT(Config_Option_GetBaseValue(fov)->as_int, 65);

    Config_Option_PopHold(fov);
    CHECK_EQ_INT(Config_Option_GetBaseValue(fov)->as_int, 65);
}

TEST(holds_stack_and_lift_one_at_a_time)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 },
        CONFIG_HOLD_GAME_FLOW);
    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 120 }, CONFIG_HOLD_DEMO);
    CHECK_EQ_INT(m_Fov, 120);

    CHECK(Config_Option_PopHold(fov));
    CHECK_EQ_INT(m_Fov, 90);
    CHECK(Config_Option_IsHeld(fov));

    CHECK(Config_Option_PopHold(fov));
    CHECK_EQ_INT(m_Fov, 65);
    CHECK(!Config_Option_IsHeld(fov));
}

TEST(the_players_value_survives_however_many_holds_went_over_it)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 },
        CONFIG_HOLD_GAME_FLOW);
    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 120 },
        CONFIG_HOLD_SCRIPT);
    CHECK_EQ_INT(Config_Option_GetBaseValue(fov)->as_int, 65);
}

TEST(the_stack_has_a_bottom)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    for (int32_t i = 0; i < CONFIG_HOLD_MAX_DEPTH; i++) {
        M_Hold(
            fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 + i },
            CONFIG_HOLD_SCRIPT);
    }
    CHECK(!Config_Option_PushHold(
        fov, &(TRX_VALUE) { .type = TVT_S32, .as_int = 130 },
        CONFIG_HOLD_SCRIPT));
    CHECK_EQ_INT(m_Fov, 90 + CONFIG_HOLD_MAX_DEPTH - 1);
}

TEST(releasing_what_nothing_holds_does_nothing)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");
    CHECK(!Config_Option_PopHold(fov));
    CHECK_EQ_INT(m_Fov, 65);
}

TEST(a_string_option_is_restored_by_value_not_by_pointer)
{
    M_SetUp();
    // The only type that owns memory. The value underneath has to be a copy: if
    // it pointed at the option's own buffer, writing the option would overwrite
    // the value meant to be put back.
    CONFIG_OPTION *const color = M_Option("visuals.water_color");

    M_Hold(
        color, (TRX_VALUE) { .type = TVT_STRING, .as_str = "0080ff" },
        CONFIG_HOLD_SCRIPT);
    CHECK_EQ_STR(m_WaterColor, "0080ff");

    CHECK(Config_Option_PopHold(color));
    CHECK_EQ_STR(m_WaterColor, "ff0000");
}

TEST(a_bool_option_round_trips)
{
    M_SetUp();
    CONFIG_OPTION *const music = M_Option("audio.music");

    M_Hold(
        music, (TRX_VALUE) { .type = TVT_BOOL, .as_bool = false },
        CONFIG_HOLD_DEMO);
    CHECK(!m_MusicOn);

    CHECK(Config_Option_PopHold(music));
    CHECK(m_MusicOn);
}

TEST(a_write_while_held_lands_on_the_hold_and_not_underneath)
{
    M_SetUp();
    // What /set --force means: the player is changing the value in force for
    // this session, not the one the file carries.
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 },
        CONFIG_HOLD_GAME_FLOW);
    Config_Option_Write(fov, &(TRX_VALUE) { .type = TVT_S32, .as_int = 100 });
    CHECK_EQ_INT(m_Fov, 100);
    CHECK_EQ_INT(Config_Option_GetBaseValue(fov)->as_int, 65);

    Config_Option_PopHold(fov);
    CHECK_EQ_INT(m_Fov, 65);
}

TEST(a_held_option_keeps_its_default_until_a_restore_is_forced)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 },
        CONFIG_HOLD_GAME_FLOW);
    CHECK(!Config_Option_RestoreDefault(fov, false));
    CHECK_EQ_INT(m_Fov, 90);

    CHECK(Config_Option_RestoreDefault(fov, true));
    CHECK_EQ_INT(m_Fov, 80);
    CHECK_EQ_INT(Config_Option_GetBaseValue(fov)->as_int, 65);
}

TEST(options_are_held_one_at_a_time)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");
    CONFIG_OPTION *const music = M_Option("audio.music");

    M_Hold(
        fov, (TRX_VALUE) { .type = TVT_S32, .as_int = 90 }, CONFIG_HOLD_SCRIPT);
    CHECK(!Config_Option_IsHeld(music));
    CHECK(m_MusicOn);

    Config_Option_PopHold(fov);
    CHECK_EQ_INT(m_Fov, 65);
}

TEST(an_unheld_option_saves_the_value_it_holds)
{
    M_SetUp();
    CONFIG_OPTION *const fov = M_Option("visuals.fov");
    CHECK_EQ_INT(Config_Option_GetBaseValue(fov)->as_int, m_Fov);
}
