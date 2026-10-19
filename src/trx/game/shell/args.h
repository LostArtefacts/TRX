#pragma once

#include <trx/core/vector.h>
#include <trx/game/shell/mod.h>

typedef struct {
    int32_t engine_version;

    bool check_lua_api;
    const SHELL_MOD *mod;
    // The game the player named on the command line, and whether they named
    // one at all. A named game that cannot be played is an error rather than
    // something to quietly pick around.
    const char *mod_request;
    bool mod_explicit;
    struct {
        int32_t num;
        const char *query;
        const char *path;
    } level_request;
    // The save to start from, counted from 0, or -1 for none. A quick save is
    // counted in its order on screen, the most recent first.
    int32_t save_to_load;
    bool save_is_quick;
} STARTUP_SETTINGS;

typedef struct {
    // Owned argv snapshot used as backing storage for pointer-valued fields
    // (e.g. level/replay/test paths). Freed by Shell_FreeArgs.
    VECTOR *original_args;

    STARTUP_SETTINGS startup;
    const char *test_record_path;
    const char *test_replay_path;
    bool headless;
    bool debug_render_performance;
    int32_t headless_fps; // in headless mode, force fixed fps (0 = unlocked)
    bool quiet;
} SHELL_ARGS;

// Adopts `raw_args`.
// Requirements:
// - `raw_args` items must be `char *` allocated on heap (or nullptr).
// - ownership of vector + strings transfers to returned SHELL_ARGS.
// - caller must not free or mutate `raw_args` after this call.
RESULT Shell_ParseArgs(VECTOR *raw_args, SHELL_ARGS **out_args);

// Reads a save slot as the console's load command takes it, into the settings.
// A number names a numbered slot. q or quick, with an optional number, names a
// quick save by its order on screen. Both count from 1. Returns false and
// leaves the settings alone where the text names no slot.
bool Shell_ParseSaveSlot(const char *text, STARTUP_SETTINGS *settings);
// Frees SHELL_ARGS and its adopted `original_args` vector + strings.
void Shell_FreeArgs(SHELL_ARGS *args);
