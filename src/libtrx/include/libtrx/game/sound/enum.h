#pragma once

// clang-format off
typedef enum {
    SPM_NORMAL     = 0,
    SPM_UNDERWATER = 1,
    SPM_ALWAYS     = 2,
#if TR_VERSION == 2
    SPM_PITCH      = 4,
#endif
} SOUND_PLAY_MODE;
// clang-format on
