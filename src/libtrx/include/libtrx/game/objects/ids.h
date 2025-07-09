#pragma once

#define O_FIRST 0
#if TR_VERSION == 1
    #define O_ORIGINAL_NUMBER_OF 191
#else
    #define O_ORIGINAL_NUMBER_OF 265
#endif

typedef enum {
    NO_OBJECT = -1,
#define OBJ_ID_DEFINE(game_id, uuid_str, enum_value)                           \
    enum_value = O_FIRST + game_id,
#include "./ids.def"
#undef OBJ_ID_DEFINE
    // sentinel
    O_NUMBER_OF,
} GAME_OBJECT_ID;
