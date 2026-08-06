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
// How many times the listener below was told that the FOV moved.
static int32_t m_FovTold;

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

// A listener that moves a setting of its own, as a script's config watcher
// does. The cap is there because a report told again would tell it again, on
// and on, and a test that hangs says less than one that counts.
static void M_MoveAnother(const EVENT *const event, void *const user_data)
{
    if (!Config_Change_HasMirror(event->data, &m_Fov)) {
        return;
    }
    m_FovTold++;
    if (m_FovTold > 4) {
        return;
    }
    CONFIG_SET(m_MusicOn, false);
    Config_Update();
}

// The report is spent once. A listener that moves a setting of its own is
// heard as a report of its own, rather than being told this one over again.
TEST(a_listener_that_changes_a_setting_is_not_told_the_same_report_twice)
{
    M_SetUp();
    m_FovTold = 0;
    const int32_t listener = Config_SubscribeChanges(M_MoveAnother, nullptr);

    CHECK(Config_SetValue(&m_Fov, Value_Of(90)));
    CHECK(Config_Update());
    CHECK_EQ_INT(m_FovTold, 1);

    Config_UnsubscribeChanges(listener);
}
