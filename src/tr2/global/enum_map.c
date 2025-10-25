#include "game/game_flow/types.h"

#include <libtrx/config/types.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/input.h>
#include <libtrx/game/objects/ids.h>
#include <libtrx/gfx/common.h>
#include <libtrx/screenshot.h>

void EnumMap_Init(void)
{
#include "global/enum_map.def"

#include <libtrx/game/enum_map.def>
}
