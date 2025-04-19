#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_flow/common.h"
#include "game/game_string.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/rooms.h"
#include "memory.h"
#include "strings.h"

#include <string.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level->type == GFL_TITLE) {
        return CR_UNAVAILABLE;
    }

    const char *level_type_fmt = nullptr;
    int32_t reindex = 0;
    switch (current_level->type) {
    case GFL_CUTSCENE:
        level_type_fmt = GS(OSD_POS_LEVEL_FMT_CUTSCENE);
        reindex = 1;
        break;
    case GFL_DEMO:
        level_type_fmt = GS(OSD_POS_LEVEL_FMT_DEMO);
        reindex = 1;
        break;
    default:
        level_type_fmt = GS(OSD_POS_LEVEL_FMT);
        reindex = GF_GetGymLevel() == nullptr ? 1 : 0;
        break;
    }

    char *level_type =
        String_Format(level_type_fmt, current_level->num + reindex);

    const ITEM *const lara_item = Lara_GetItem();
    int16_t room_num = lara_item->room_num;
    const ROOM *const room = Room_Get(room_num);
    if (Room_GetFlipStatus() && room->flipped_room != NO_ROOM_NEG) {
        room_num = room->flipped_room;
    }
    char *details = lara_item == nullptr
        ? String_Format("%s", GS(OSD_POS_LARA_MISSING))
        : String_Format(
              GS(OSD_POS_LARA_POS_FMT), room_num,
              lara_item->pos.x / (float)WALL_L,
              lara_item->pos.y / (float)WALL_L,
              lara_item->pos.z / (float)WALL_L,
              lara_item->rot.x * 360.0f / (float)DEG_360,
              lara_item->rot.y * 360.0f / (float)DEG_360,
              lara_item->rot.z * 360.0f / (float)DEG_360);
    const char *const glue = lara_item == nullptr ? "\n" : "  ";

    char *message = strcmp(level_type, current_level->title) == 0
        ? String_Format("%s%s%s", level_type, glue, details)
        : String_Format(
              "%s (%s)%s%s", level_type, current_level->title, glue, details);

    Console_Log("%s", message);

    Memory_FreePointer(&details);
    Memory_FreePointer(&message);
    Memory_FreePointer(&level_type);

    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("pos", M_Entrypoint)
