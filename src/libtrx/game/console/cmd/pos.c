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

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsLoaded()) {
        return CR_UNAVAILABLE;
    }

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

    const char *const level_type =
        String_FormatStatic(level_type_fmt, current_level->num + reindex);

    const ITEM *const lara_item = Lara_GetItem();
    const char *details;
    if (lara_item == nullptr) {
        details = String_FormatStatic("%s", GS(OSD_POS_LARA_MISSING));
    } else {
        int16_t room_num = lara_item->room_num;
        const ROOM *const room = Room_Get(room_num);
        if (Room_GetFlipStatus() && room->flipped_room != NO_ROOM) {
            room_num = room->flipped_room;
        }
        details = String_FormatStatic(
            GS(OSD_POS_LARA_POS_FMT), room_num,
            lara_item->pos.x / (float)WALL_L, lara_item->pos.y / (float)WALL_L,
            lara_item->pos.z / (float)WALL_L,
            lara_item->rot.x * 360.0f / (float)DEG_360,
            lara_item->rot.y * 360.0f / (float)DEG_360,
            lara_item->rot.z * 360.0f / (float)DEG_360);
    }
    const char *const glue = lara_item == nullptr ? "\n" : "  ";

    const char *const message = current_level->title != nullptr
            && strcmp(level_type, current_level->title) == 0
        ? String_FormatStatic("%s%s%s", level_type, glue, details)
        : String_FormatStatic(
              "%s (%s)%s%s", level_type, current_level->title, glue, details);

    Console_Log("%s", message);

    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("pos", M_Entrypoint, GS_ID(CONSOLE_HELP_POS))
