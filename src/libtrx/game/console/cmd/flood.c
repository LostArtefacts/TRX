#include "game/console/registry.h"
#include "game/lara/common.h"
#include "game/rooms.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return CR_UNAVAILABLE;
    }

    if (String_Equivalent(ctx->prefix, "flood")) {
        Room_Get(lara_item->room_num)->flags |= RF_UNDERWATER;
    } else if (String_Equivalent(ctx->prefix, "drain")) {
        Room_Get(lara_item->room_num)->flags &= ~RF_UNDERWATER;
    } else {
        return CR_UNAVAILABLE;
    }

    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("flood|drain", M_Entrypoint)
