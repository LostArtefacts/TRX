#pragma once

#include "../../vector.h"

typedef enum {
    SHELL_MOD_TR1_OG,
    SHELL_MOD_TR1_UB,
    SHELL_MOD_TR1_DEMO_PC,
    SHELL_MOD_TR1_CUSTOM_LEVEL,
    SHELL_MOD_TR2_OG,
    SHELL_MOD_TR2_GM,
    SHELL_MOD_TR2_CUSTOM_LEVEL,
} SHELL_MOD;

typedef struct {
    VECTOR *original_args;

    SHELL_MOD mod;
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

const char *Shell_GetCommonStringsPath(void);
const char *Shell_GetBaseGameStringsPath(void);
const char *Shell_GetGameStringsPath(SHELL_MOD mod);
const char *Shell_GetGameFlowPath(SHELL_MOD mod);
