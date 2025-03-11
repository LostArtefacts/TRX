#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_flow/common.h"
#include "game/game_string.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    const OBJECT *const obj = Object_Get(O_LARA);
    if (!obj->loaded) {
        return CR_UNAVAILABLE;
    }

    const ITEM *const lara_item = Lara_GetItem();

    const char *prefix = nullptr;
    int32_t reindex = 0;
    switch (GF_GetCurrentLevel()->type) {
    case GFL_CUTSCENE:
        prefix = GS(OSD_POS_CUTSCENE);
        reindex = 1;
        break;
    case GFL_DEMO:
        prefix = GS(OSD_POS_DEMO);
        reindex = 1;
        break;
    default:
        prefix = GS(OSD_POS_LEVEL);
        reindex = GF_GetGymLevel() == nullptr ? 1 : 0;
        break;
    }

    // clang-format off
    Console_Log(
        GS(OSD_POS_GET),
        prefix,
        GF_GetCurrentLevel()->num + reindex,
        GF_GetCurrentLevel()->title,
        lara_item->room_num,
        lara_item->pos.x / (float)WALL_L,
        lara_item->pos.y / (float)WALL_L,
        lara_item->pos.z / (float)WALL_L,
        lara_item->rot.x * 360.0f / (float)DEG_360,
        lara_item->rot.y * 360.0f / (float)DEG_360,
        lara_item->rot.z * 360.0f / (float)DEG_360);
    // clang-format on

    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("pos", M_Entrypoint)
