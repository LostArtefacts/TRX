// Writing a setting by value. A caller states a value in the shape its own C
// expression had, and the option says what it was meant as: an enumeration
// constant is an integer, a whole number written for a float setting is an
// integer, and a string written for a dynamic enum is a string.

#include <harness/harness.h>

#include <trx/config/common.h>
#include <trx/config/priv.h>
#include <trx/config/registry.h>

static bool m_MusicOn;
static double m_Fov;
static char *m_Outfit;
static bool m_LastPersist;

static void M_SetUp(void)
{
    // Each test starts from the same three options, as a game change would
    // leave them.
    Config_DropAllOptions();
    Config_Register(&(CONFIG_OPTION_DESC) {
        .name = "audio.music",
        .default_value = { .type = TVT_BOOL, .as_bool = true },
        .mirror = &m_MusicOn });
    Config_Register(&(CONFIG_OPTION_DESC) {
        .name = "visuals.fov",
        .default_value = { .type = TVT_DOUBLE, .as_num = 80.0 },
        .mirror = &m_Fov });
    Config_Register(&(CONFIG_OPTION_DESC) {
        .name = "visuals.lara_outfit",
        .default_value = { .type = TVT_DYNAMIC_ENUM, .as_str = "default" },
        .mirror = &m_Outfit });
}

TEST(a_whole_number_written_for_a_float_setting_is_taken_as_one)
{
    M_SetUp();
    CHECK(Config_SetValue(&m_Fov, Value_Of(90)));
    CHECK_EQ_INT((int32_t)m_Fov, 90);
}

TEST(a_string_written_for_a_dynamic_enum_is_taken_as_one)
{
    M_SetUp();
    CHECK(Config_SetValue(&m_Outfit, Value_Of("wetsuit")));
    CHECK_EQ_STR(m_Outfit, "wetsuit");
}

TEST(a_toggle_flips_a_bool_setting)
{
    M_SetUp();
    CHECK(CONFIG_TOGGLE(m_MusicOn));
    CHECK(!m_MusicOn);
    CHECK(CONFIG_TOGGLE(m_MusicOn));
    CHECK(m_MusicOn);
}

static void M_RecordChange(const EVENT *const event, void *const user_data)
{
    m_LastPersist = ((const CONFIG_CHANGE *)event->data)->persist;
}

// A hold lives as long as whatever put it there. What lands on one is the
// game flow's, a script's or a demo's doing, so it is never the file's to keep.
TEST(a_write_while_held_is_not_the_players_to_save)
{
    M_SetUp();
    const int32_t listener = Config_SubscribeChanges(M_RecordChange, nullptr);

    CHECK(Config_SetValue(&m_Fov, Value_Of(90)));
    m_LastPersist = false;
    CHECK(Config_Update());
    CHECK(m_LastPersist);

    CHECK(Config_PushHold(&m_Fov, Value_Of(120), CONFIG_HOLD_GAME_FLOW));
    CHECK(Config_SetValue(&m_Fov, Value_Of(110)));
    m_LastPersist = true;
    CHECK(Config_Update());
    CHECK(!m_LastPersist);

    Config_UnsubscribeChanges(listener);
}
