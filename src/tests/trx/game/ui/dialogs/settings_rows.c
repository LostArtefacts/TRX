// Where a settings row sits. The rows a .def names come out in the order they
// are written in, and a row a script declares lands where the anchor it named
// puts it - which is a number from that point on, so reordering the tab or
// dropping the anchor cannot leave it pointing at nothing.
//
// The tab used here is the sound volume one, of which only the first three rows
// are registered: a tab naming a setting this game does not have is a row that
// is not shown, which is what a game without that option looks like.

#include <harness/harness.h>

#include <trx/config/priv.h>
#include <trx/config/registry.h>
#include <trx/game/ui/dialogs/settings_rows.h>

#include <stdio.h>
#include <string.h>

#define M_TAB CONFIG_TAB_SOUND_VOLUME

static double m_MasterVolume;
static double m_SoundVolume;
static double m_MusicVolume;
// What the options a script would declare are kept in. Every option needs a
// mirror of its own, and what is in them is not what these tests are about.
static double m_Declared[200];

static void M_Register(const char *const name, void *const mirror)
{
    CHECK(
        Config_Register(&(CONFIG_OPTION_DESC) {
            .name = name,
            .default_value = { .type = TVT_DOUBLE, .as_num = 1.0 },
            .mirror = mirror })
        != nullptr);
}

static void M_SetUp(void)
{
    UI_Settings_DropDeclaredRows();
    Config_DropAllOptions();
    M_Register("audio.master_volume", &m_MasterVolume);
    M_Register("audio.sound_volume", &m_SoundVolume);
    M_Register("audio.music_volume", &m_MusicVolume);
    M_Register("mod.scanlines", &m_Declared[0]);
}

// Which row shows the option a name names, by position in the tab.
static int32_t M_RowIndex(const char *const name)
{
    for (int32_t i = 0; i < UI_Settings_GetRowCount(M_TAB); i++) {
        if (strcmp(UI_Settings_GetRow(M_TAB, i)->option->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

TEST(a_tab_shows_the_rows_it_names_that_this_game_has)
{
    M_SetUp();
    CHECK_EQ_INT(UI_Settings_GetRowCount(M_TAB), 3);
    CHECK_EQ_INT(M_RowIndex("audio.master_volume"), 0);
    CHECK_EQ_INT(M_RowIndex("audio.sound_volume"), 1);
    CHECK_EQ_INT(M_RowIndex("audio.music_volume"), 2);
}

TEST(a_declared_row_sits_after_the_row_it_names)
{
    M_SetUp();
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.scanlines", nullptr, "audio.sound_volume");
    CHECK_EQ_INT(M_RowIndex("mod.scanlines"), 2);
}

TEST(a_declared_row_sits_before_the_row_it_names)
{
    M_SetUp();
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.scanlines", "audio.master_volume", nullptr);
    CHECK_EQ_INT(M_RowIndex("mod.scanlines"), 0);
}

// An anchor is a name, and the name the settings file keys an option on is the
// last segment of it. Both spellings reach the same row.
TEST(an_anchor_names_a_row_by_either_spelling)
{
    M_SetUp();
    UI_Settings_AddDeclaredRow(M_TAB, "mod.scanlines", nullptr, "sound_volume");
    CHECK_EQ_INT(M_RowIndex("mod.scanlines"), 2);
}

TEST(a_declared_row_naming_no_anchor_lands_at_the_end)
{
    M_SetUp();
    UI_Settings_AddDeclaredRow(M_TAB, "mod.scanlines", nullptr, nullptr);
    CHECK_EQ_INT(M_RowIndex("mod.scanlines"), 3);
}

// A tab lists rows for every game, so an anchor naming a row this tab does not
// show is a row that cannot be placed against it rather than an error.
TEST(a_declared_row_naming_an_anchor_the_tab_does_not_show_lands_at_the_end)
{
    M_SetUp();
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.scanlines", nullptr, "audio.fmv_volume");
    CHECK_EQ_INT(M_RowIndex("mod.scanlines"), 3);
}

TEST(rows_declared_against_one_anchor_read_in_the_order_they_arrived)
{
    M_SetUp();
    M_Register("mod.grain", &m_Declared[1]);
    M_Register("mod.vignette", &m_Declared[2]);
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.scanlines", nullptr, "audio.master_volume");
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.grain", nullptr, "audio.master_volume");
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.vignette", nullptr, "audio.master_volume");

    CHECK_EQ_INT(M_RowIndex("mod.scanlines"), 1);
    CHECK_EQ_INT(M_RowIndex("mod.grain"), 2);
    CHECK_EQ_INT(M_RowIndex("mod.vignette"), 3);
    CHECK_EQ_INT(M_RowIndex("audio.sound_volume"), 4);
}

// A tab has room for a hundred rows between any two of its own before it has to
// be numbered again, and the numbering has to leave the order alone.
TEST(a_tab_that_runs_out_of_room_is_numbered_again)
{
    M_SetUp();
    for (int32_t i = 0; i < 200; i++) {
        char name[32];
        snprintf(name, sizeof(name), "mod.filter_%d", i);
        M_Register(name, &m_Declared[i]);
        UI_Settings_AddDeclaredRow(M_TAB, name, "audio.sound_volume", nullptr);
    }

    CHECK_EQ_INT(UI_Settings_GetRowCount(M_TAB), 203);
    CHECK_EQ_INT(M_RowIndex("audio.master_volume"), 0);
    CHECK_EQ_INT(M_RowIndex("mod.filter_0"), 1);
    CHECK_EQ_INT(M_RowIndex("mod.filter_199"), 200);
    CHECK_EQ_INT(M_RowIndex("audio.sound_volume"), 201);
}

TEST(dropping_the_declared_rows_leaves_the_tab_as_its_def_names_it)
{
    M_SetUp();
    UI_Settings_AddDeclaredRow(
        M_TAB, "mod.scanlines", nullptr, "audio.master_volume");
    CHECK_EQ_INT(UI_Settings_GetRowCount(M_TAB), 4);

    UI_Settings_DropDeclaredRows();
    CHECK_EQ_INT(UI_Settings_GetRowCount(M_TAB), 3);
    CHECK_EQ_INT(M_RowIndex("audio.master_volume"), 0);
}
