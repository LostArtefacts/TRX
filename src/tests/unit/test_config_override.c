// The runtime override stack, against a config of three options and nothing
// else. No game, no settings file, no option map: the stack asks the config
// layer for those two things (where an option lives, how a string becomes a
// value) and this file answers.
//
// What is under test is that the player's value survives. A script holds a
// setting away from it, and when the script lets go, the player's own value is
// what comes back - never the default, and never the value some other script
// pushed.

#include "harness.h"

#include <trx/config/override.h>
#include <trx/config/value.h>

#include <stdlib.h>
#include <string.h>

// The player's config: what they chose, in the three shapes that behave
// differently - a bool, a number, and a string, which is the one that owns
// memory.
static bool m_MusicOn;
static int32_t m_Fov;
static char *m_WaterColor;

static bool m_Enforced;

static const CONFIG_OPTION m_Options[] = {
    { .name = "audio.music", .type = TVT_BOOL, .target = &m_MusicOn },
    { .name = "visuals.fov", .type = TVT_S32, .target = &m_Fov },
    { .name = "visuals.water_color",
      .type = TVT_STRING,
      .target = &m_WaterColor },
    { nullptr },
};

static const CONFIG_OPTION *M_Option(const char *const name)
{
    for (const CONFIG_OPTION *o = m_Options; o->name != nullptr; o++) {
        if (strcmp(o->name, name) == 0) {
            return o;
        }
    }
    return nullptr;
}

// The two things the stack asks the config layer for. The real parser lives in
// config/common.c, which drags in the game; this one reads the same spellings.
const CONFIG_OPTION *Config_GetOption(const void *const target)
{
    for (const CONFIG_OPTION *o = m_Options; o->name != nullptr; o++) {
        if (o->target == target) {
            return o;
        }
    }
    return nullptr;
}

bool Config_IsOptionEnforced(const void *const target)
{
    return m_Enforced;
}

bool Config_SetOptionValueFromString(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    if (Config_IsOptionEnforced(option->target)) {
        return false;
    }
    switch (option->type) {
    case TVT_BOOL:
        if (strcmp(new_value, "true") == 0) {
            *(bool *)option->target = true;
            return true;
        }
        if (strcmp(new_value, "false") == 0) {
            *(bool *)option->target = false;
            return true;
        }
        return false;

    case TVT_S32: {
        char *end = nullptr;
        const long parsed = strtol(new_value, &end, 10);
        if (end == new_value || *end != '\0') {
            return false;
        }
        *(int32_t *)option->target = (int32_t)parsed;
        return true;
    }

    case TVT_STRING: {
        char **const p = (char **)option->target;
        free(*p);
        *p = strdup(new_value);
        return true;
    }

    default:
        return false;
    }
}

static void M_SetUp(void)
{
    ConfigOverride_Clear();
    m_Enforced = false;
    m_MusicOn = true;
    m_Fov = 65;
    free(m_WaterColor);
    m_WaterColor = strdup("ff0000");
}

TEST(an_override_holds_the_option_and_a_restore_gives_it_back)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");

    CHECK(ConfigOverride_PushFromString(fov, "90"));
    CHECK_EQ_INT(m_Fov, 90);
    CHECK(ConfigOverride_IsOverridden(fov));

    CHECK(ConfigOverride_Pop(fov));
    // The pop puts the player's own 65 back, not the default and not the 90.
    CHECK_EQ_INT(m_Fov, 65);
    CHECK(!ConfigOverride_IsOverridden(fov));
}

TEST(the_value_underneath_is_the_players_own_not_the_one_being_pushed)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");

    ConfigOverride_PushFromString(fov, "90");
    CHECK_EQ_INT(*(const int32_t *)ConfigOverride_GetBaseValuePtr(fov), 65);

    ConfigOverride_Pop(fov);
    CHECK_EQ_INT(m_Fov, 65);
}

TEST(overrides_stack_and_each_pop_lifts_one)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");

    ConfigOverride_PushFromString(fov, "90");
    ConfigOverride_PushFromString(fov, "120");
    CHECK_EQ_INT(m_Fov, 120);

    // The one underneath is the other override, not the player's value.
    CHECK(ConfigOverride_Pop(fov));
    CHECK_EQ_INT(m_Fov, 90);
    CHECK(ConfigOverride_IsOverridden(fov));

    CHECK(ConfigOverride_Pop(fov));
    CHECK_EQ_INT(m_Fov, 65);
    CHECK(!ConfigOverride_IsOverridden(fov));
}

TEST(the_base_value_survives_however_deep_the_stack_goes)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");

    ConfigOverride_PushFromString(fov, "90");
    ConfigOverride_PushFromString(fov, "120");
    CHECK_EQ_INT(*(const int32_t *)ConfigOverride_GetBaseValuePtr(fov), 65);
}

TEST(the_stack_has_a_floor)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");

    ConfigOverride_PushFromString(fov, "90");
    ConfigOverride_PushFromString(fov, "100");
    ConfigOverride_PushFromString(fov, "110");
    // Depth is capped, so a runaway script cannot grow it without limit.
    CHECK(!ConfigOverride_PushFromString(fov, "120"));
    CHECK_EQ_INT(m_Fov, 110);
}

TEST(popping_what_was_never_pushed_says_so)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");
    CHECK(!ConfigOverride_Pop(fov));
    CHECK_EQ_INT(m_Fov, 65);
}

TEST(a_string_option_is_restored_by_value_not_by_pointer)
{
    M_SetUp();
    // The only type that owns memory. The base value has to be a copy: if it
    // pointed at the option's own buffer, overwriting the option would
    // overwrite the value meant to be put back.
    const CONFIG_OPTION *const color = M_Option("visuals.water_color");

    CHECK(ConfigOverride_PushFromString(color, "0080ff"));
    CHECK_EQ_STR(m_WaterColor, "0080ff");

    CHECK(ConfigOverride_Pop(color));
    CHECK_EQ_STR(m_WaterColor, "ff0000");
}

TEST(a_bool_option_round_trips)
{
    M_SetUp();
    const CONFIG_OPTION *const music = M_Option("audio.music");

    CHECK(ConfigOverride_PushFromString(music, "false"));
    CHECK(!m_MusicOn);

    CHECK(ConfigOverride_Pop(music));
    CHECK(m_MusicOn);
}

TEST(a_value_that_will_not_parse_leaves_the_option_alone)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");

    CHECK(!ConfigOverride_PushFromString(fov, "wide"));
    CHECK_EQ_INT(m_Fov, 65);
    CHECK(!ConfigOverride_IsOverridden(fov));
}

TEST(an_option_the_game_flow_enforces_cannot_be_overridden)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");
    m_Enforced = true;

    CHECK(!ConfigOverride_PushFromString(fov, "90"));
    CHECK_EQ_INT(m_Fov, 65);
    CHECK(!ConfigOverride_IsOverridden(fov));
}

TEST(options_are_overridden_one_at_a_time)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");
    const CONFIG_OPTION *const music = M_Option("audio.music");

    ConfigOverride_PushFromString(fov, "90");
    CHECK(!ConfigOverride_IsOverridden(music));
    CHECK(m_MusicOn);

    ConfigOverride_Pop(fov);
    CHECK_EQ_INT(m_Fov, 65);
}

TEST(an_unoverridden_option_saves_its_own_value)
{
    M_SetUp();
    const CONFIG_OPTION *const fov = M_Option("visuals.fov");
    CHECK_EQ_INT(ConfigOverride_GetBaseValuePtr(fov), (const void *)&m_Fov);
}
