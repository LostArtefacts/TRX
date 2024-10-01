#pragma once

typedef enum {
    NO_OBJECT = -1,
#define OBJ_ID_DEFINE(object_id, enum_value) object_id = enum_value,
#include "./ids.def"
    O_NUMBER_OF,
} GAME_OBJECT_ID;
