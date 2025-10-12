#pragma once

#define O_FIRST 0
#if TR_VERSION == 1
    #define O_ORIGINAL_NUMBER_OF 191
#else
    #define O_ORIGINAL_NUMBER_OF 265
#endif

typedef enum {
    NO_OBJECT = -1,
#define X_OBJ_ID_DEFINE(uuid_str, enum_value, game1_id, game2_id) enum_value,
#include "./ids.def"
#undef X_OBJ_ID_DEFINE
    // sentinel
    O_NUMBER_OF,
} GAME_OBJECT_ID;
