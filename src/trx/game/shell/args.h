#pragma once

#include <trx/core/vector.h>
#include <trx/game/shell/mod.h>

typedef struct {
    VECTOR *original_args;

    int32_t engine_version;
    const SHELL_MOD *mod;
    int32_t level_to_select;
    const char *level_to_play;
    int32_t save_to_load;
    const char *test_record_path;
    const char *test_replay_path;
    bool headless;
    bool debug_render_performance;
    int32_t headless_fps; // in headless mode, force fixed fps (0 = unlocked)
    bool quiet;
} SHELL_ARGS;

SHELL_ARGS *Shell_ParseArgs(VECTOR *raw_args);
