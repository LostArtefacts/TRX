#pragma once

#define O_FIRST 0

typedef enum {
    NO_OBJECT = -1,
#define OBJ_ID_DEFINE(object_id, enum_value) object_id = O_FIRST + enum_value,
#include "./ids.def"
    O_NUMBER_OF,
} GAME_OBJECT_ID;
