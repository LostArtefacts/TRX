#include "global/types.h"

#include <libtrx/enum_map.h>
#include <libtrx/game/objects/ids.h>

void EnumMap_Init(void)
{
#include "global/enum_map.def"

#undef OBJ_ID_DEFINE
#define OBJ_ID_DEFINE(object_id, value)                                        \
    EnumMap_Define("GAME_OBJECT_ID", object_id, #object_id);
#include <libtrx/game/objects/ids.def>
}
