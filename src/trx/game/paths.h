#pragma once

#include <trx/core/filesystem.h>
#include <trx/game/shell/args.h>

// Game path module.
// This layer owns high-level path policy: token expansion (%trx_dir%),
// mod/base fallback order, case-aware canonicalization, and extension
// guessing/caching.
//
// Use these APIs for game asset/config resolution instead of ad-hoc file
// probes.

// clang-format off
#define GAME_PATH_DIR_LIST(X)                                                  \
    X(TRX_DIR,          trx_dir,          "%trx_dir%")                         \
    X(CONFIG_DIR,       config_dir,       "%config_dir%")                      \
    X(CACHE_DIR,        cache_dir,        "%cache_dir%")                      \
    X(GAMES_DIR,        games_dir,        "%games_dir%")                       \
    X(SCREENSHOTS_DIR,  screenshots_dir,  "%screenshots_dir%")                 \
    X(SAVES_DIR,        saves_dir,        "%saves_dir%")                       \
    X(LEGACY_SAVES_DIR, legacy_saves_dir, "%legacy_saves_dir%")                                                            \
    // clang-format on

typedef enum {
#define M_DIR_ENUM(name, field, token) GAME_PATH_##name,
    GAME_PATH_DIR_LIST(M_DIR_ENUM)
#undef M_DIR_ENUM
} GAME_PATH;

typedef enum {
    GAME_DYNAMIC_PATH_COMMON_CONFIG,
    GAME_DYNAMIC_PATH_CATALOG,
    GAME_DYNAMIC_PATH_GAMEFLOW_FILE,
    GAME_DYNAMIC_PATH_SHADER_FILE,
    GAME_DYNAMIC_PATH_FMV_FILE,
    GAME_DYNAMIC_PATH_LEVEL_FILE,
    GAME_DYNAMIC_PATH_SHARED_LEVEL_FILE,
    GAME_DYNAMIC_PATH_IMAGE_FILE,
    GAME_DYNAMIC_PATH_INJECTION_FILE,
    // A level's script, in the scripts/ of the game the level belongs to and
    // nowhere else. A game that extends another brings its own: a level of the
    // base game is not the same level once an expansion has changed what is in
    // it.
    GAME_DYNAMIC_PATH_LEVEL_SCRIPT_FILE,
    // A game's _game.lua, falling back to the game it extends: an expansion
    // with nothing of its own to set up runs the script of the game it sits on
    // top of.
    GAME_DYNAMIC_PATH_GAME_SCRIPT_FILE,
    // The two a script is required from, each naming one modules/ directory
    // and falling back nowhere. A required name carries the directory it lives
    // in, so there is nothing to choose between: a game named as it sits in
    // games/, or the pool beside the engine. Neither reaches scripts/, which
    // holds what the engine runs.
    GAME_DYNAMIC_PATH_GAME_MODULE_FILE,
    GAME_DYNAMIC_PATH_COMMON_MODULE_FILE,
    GAME_DYNAMIC_PATH_SFX_FILE,
    GAME_DYNAMIC_PATH_CDAUDIO_FILE,
    GAME_DYNAMIC_PATH_MUSIC_DIR,
    GAME_DYNAMIC_PATH_SCREENSHOT_WRITE_FILE,
    GAME_DYNAMIC_PATH_NUMBER_OF,
} GAME_DYNAMIC_PATH;

// Initialize resolver state from shell args and environment variables.
// Safe to call multiple times; later calls refresh context-derived values.
void GamePath_Init(const SHELL_ARGS *args);

// Expand `%token%` variables in an arbitrary string.
// Returns an owning string; caller must free.
char *GamePath_ExpandVars(const char *in);

// Return configured root directory for a static GAME_PATH id.
// Returned pointer is owned by resolver context; do not free.
const char *GamePath_Get(GAME_PATH path);

// Join static root and relative path.
// Returns an owning string; caller must free.
char *GamePath_Join(GAME_PATH path, const char *rel);

// Resolve with policy fallback and existence checks.
// Returns nullptr on miss; does not log.
const char *GamePath_PeekResolve(GAME_DYNAMIC_PATH path, const char *rel);
// Same as GamePath_PeekResolve, but logs an error on miss.
const char *GamePath_TryResolve(GAME_DYNAMIC_PATH path, const char *rel);
// Same as GamePath_PeekResolve, but terminates the game on miss.
const char *GamePath_Resolve(GAME_DYNAMIC_PATH path, const char *rel);

// Resolve and open a file in one call.
// Returns nullptr if resolution/open fails.
MYFILE *GamePath_OpenFile(
    GAME_DYNAMIC_PATH path, const char *rel, FILE_OPEN_MODE mode);

// Resolve and load file contents into memory.
// On failure, sets `out_data` to nullptr and `out_size` to 0 when provided.
bool GamePath_LoadFile(
    GAME_DYNAMIC_PATH path, const char *rel, char **out_data, size_t *out_size);

// Resolve and check whether the file exists.
bool GamePath_Exists(GAME_DYNAMIC_PATH path, const char *rel);

// Guess file extension (e.g. ".mp4" vs ".rpl") with case-aware resolver cache.
// Returns an owning canonical path or nullptr if no candidate exists; caller
// must free.
char *GamePath_GuessExtension(const char *path, const char **extensions);

// Resolve a user-supplied path by trying:
// 1. absolute path as-is
// 2. current working directory for relative paths
// 3. normal TRX path policy fallback for the given dynamic path
// Returns nullptr on miss; does not log.
const char *GamePath_PeekResolveUserPath(
    GAME_DYNAMIC_PATH path, const char *input_path);
