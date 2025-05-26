#pragma once

#define O_FIRST 0

typedef enum {
    NO_OBJECT = -1,
// Step 1 - define core object IDs
#define OBJ_ID_DEFINE(game_id, uuid_str, enum_value)                           \
    enum_value = O_FIRST + game_id,
#define OBJ_ID_ALIAS_DEFINE(source_id, target_id)
#include "./ids.def"
#undef OBJ_ID_DEFINE
#undef OBJ_ID_ALIAS_DEFINE
    // sentinel
    O_NUMBER_OF,
// Step 2 - define aliases after O_NUMBER_OF has been declared
// TODO: remove after aliases are no longer necessary
#define OBJ_ID_DEFINE(game_id, uuid_str, enum_value)
#define OBJ_ID_ALIAS_DEFINE(source_id, target_id) source_id = target_id,
#include "./ids.def"
#undef OBJ_ID_DEFINE
#undef OBJ_ID_ALIAS_DEFINE
} GAME_OBJECT_ID;
