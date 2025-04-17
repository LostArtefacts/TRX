#pragma once

typedef enum {
    MUSIC_LOAD_NEVER,
    MUSIC_LOAD_NON_AMBIENT,
    MUSIC_LOAD_ALWAYS,
} MUSIC_LOAD_CONDITION;

#if TR_VERSION == 1
    #include "./types_tr1.h"
#elif TR_VERSION == 2
    #include "./types_tr2.h"
#endif
