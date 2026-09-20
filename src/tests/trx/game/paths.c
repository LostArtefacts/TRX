#include <harness/harness.h>
#include <harness/stubs_paths.h>

#include <trx/game/paths.h>
#include <trx/game/shell/args.h>

#include <stdlib.h>

// The shipped tree, which holds a games/ directory the way an install does.
#define M_SHIP_DIR TEST_SHIP_DIR

static const char *M_Init(const char *const trx_dir)
{
    static const SHELL_ARGS args = {};
    if (trx_dir == nullptr) {
        unsetenv("TRX_DIR");
    } else {
        setenv("TRX_DIR", trx_dir, 1);
    }
    GamePath_Init(&args);
    return GamePath_Get(GAME_PATH_TRX_DIR);
}

TEST(the_install_root_is_the_executables_directory)
{
    CHECK_EQ_STR(M_Init(nullptr), FAKE_BASE_PATH);
}

TEST(trx_dir_replaces_the_executables_directory)
{
    CHECK_EQ_STR(M_Init(M_SHIP_DIR), M_SHIP_DIR);
}

// What a package needs: one variable moves the whole tree, and the directories
// that default below it follow it there rather than staying by the executable.
TEST(the_default_directories_follow_trx_dir)
{
    M_Init(M_SHIP_DIR);
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_GAMES_DIR), M_SHIP_DIR "/games");
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_CONFIG_DIR), M_SHIP_DIR "/cfg");
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_CACHE_DIR), M_SHIP_DIR "/cache");
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_SAVES_DIR), M_SHIP_DIR "/saves");
    CHECK_EQ_STR(
        GamePath_Get(GAME_PATH_SCREENSHOTS_DIR), M_SHIP_DIR "/screenshots");
}

TEST(trx_dir_keeps_no_trailing_separator)
{
    CHECK_EQ_STR(M_Init(M_SHIP_DIR "/"), M_SHIP_DIR);
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_GAMES_DIR), M_SHIP_DIR "/games");
}

TEST(an_empty_trx_dir_leaves_the_executables_directory)
{
    CHECK_EQ_STR(M_Init(""), FAKE_BASE_PATH);
}

TEST(a_directory_of_its_own_wins_over_trx_dir)
{
    setenv("TRX_SAVES_DIR", "/elsewhere/saves", 1);
    M_Init(M_SHIP_DIR);
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_SAVES_DIR), "/elsewhere/saves");
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_CACHE_DIR), M_SHIP_DIR "/cache");
    unsetenv("TRX_SAVES_DIR");
}

// A games directory that is not there leaves the games where the configuration
// is, which is the layout TRX shipped before games/ existed.
TEST(a_missing_games_directory_falls_back_to_the_config_directory)
{
    M_Init(M_SHIP_DIR "/cfg");
    CHECK_EQ_STR(GamePath_Get(GAME_PATH_GAMES_DIR), M_SHIP_DIR "/cfg/cfg");
}
