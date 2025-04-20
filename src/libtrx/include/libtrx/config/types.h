#pragma once

#include "./const.h"

#include <stdint.h>

typedef enum {
    MUSIC_LOAD_NEVER,
    MUSIC_LOAD_NON_AMBIENT,
    MUSIC_LOAD_ALWAYS,
} MUSIC_LOAD_CONDITION;

typedef enum {
    UI_STYLE_PS1,
    UI_STYLE_PC,
} UI_STYLE;

typedef struct {
    struct {
        uint32_t time;
        uint32_t attempt_num;
    } entries[MAX_ASSAULT_TIMES];
    int32_t best_time;
    uint32_t total_attempts;
} ASSAULT_STATS;

#if TR_VERSION == 1
    #include "./types_tr1.h"
#elif TR_VERSION == 2
    #include "./types_tr2.h"
#endif
