#pragma once

#include <stdint.h>

typedef enum {
    GF_CONTINUE_SEQUENCE,
    GF_START_GAME,
    GF_START_CINE,
    GF_START_FMV,
    GF_START_DEMO,
    GF_EXIT_TO_TITLE,
    GF_LEVEL_COMPLETE,
    GF_EXIT_GAME,
    GF_START_SAVED_GAME,
    GF_RESTART_GAME,
    GF_SELECT_GAME,
    GF_START_GYM,
    GF_STORY_SO_FAR,
} GAMEFLOW_ACTION;

typedef enum {
#if TR_VERSION == 2
    GFL_NO_LEVEL = -1,
#endif
    GFL_TITLE = 0,
    GFL_NORMAL = 1,
    GFL_SAVED = 2,
    GFL_DEMO = 3,
    GFL_CUTSCENE = 4,
#if TR_VERSION == 1
    GFL_GYM = 5,
    GFL_CURRENT = 6,
    GFL_RESTART = 7,
    GFL_SELECT = 8,
    GFL_BONUS = 9,
    GFL_TITLE_DEMO_PC = 10,
    GFL_LEVEL_DEMO_PC = 11,
#elif TR_VERSION == 2
    GFL_STORY = 5,
    GFL_QUIET = 6,
    GFL_MID_STORY = 7,
#endif
} GAMEFLOW_LEVEL_TYPE;

typedef struct GAMEFLOW_COMMAND {
    GAMEFLOW_ACTION action;
    int32_t param;
} GAMEFLOW_COMMAND;
