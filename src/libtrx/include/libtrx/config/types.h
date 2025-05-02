#pragma once

#include "./const.h"

#include <stdint.h>

typedef enum {
    BL_TOP_LEFT,
    BL_TOP_CENTER,
    BL_TOP_RIGHT,
    BL_BOTTOM_LEFT,
    BL_BOTTOM_CENTER,
    BL_BOTTOM_RIGHT,
    BL_CUSTOM,
} BAR_LOCATION;

typedef enum {
    BSM_DEFAULT,
    BSM_FLASHING_OR_DEFAULT,
    BSM_FLASHING_ONLY,
    BSM_ALWAYS,
    BSM_NEVER,
    BSM_PS1,
    BSM_BOSS_ONLY,
} BAR_SHOW_MODE;

typedef enum {
    BC_GOLD,
    BC_BLUE,
    BC_GREY,
    BC_RED,
    BC_SILVER,
    BC_GREEN,
    BC_GOLD2,
    BC_BLUE2,
    BC_PINK,
    BC_PURPLE,
} BAR_COLOR;

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
    uint32_t total_attempts;
} ASSAULT_STATS;

#if TR_VERSION == 1
    #include "./types_tr1.h"
#elif TR_VERSION == 2
    #include "./types_tr2.h"
#endif
