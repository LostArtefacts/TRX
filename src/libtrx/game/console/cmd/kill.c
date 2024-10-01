#include "game/console/cmd/kill.h"

#include "game/const.h"
#include "game/creature.h"
#include "game/game_string.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/common.h"
#include "game/lara/misc.h"
#include "game/objects/common.h"
#include "game/objects/ids.h"
#include "game/objects/names.h"
#include "game/objects/vars.h"
#include "memory.h"
#include "strings.h"

static bool M_CanTargetObjectCreature(GAME_OBJECT_ID object_id);
static COMMAND_RESULT M_KillAllEnemies(void);
static COMMAND_RESULT M_KillNearestEnemies(void);
static COMMAND_RESULT M_KillEnemyType(const char *enemy_name);
static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static bool M_CanTargetObjectCreature(const GAME_OBJECT_ID object_id)
{
    return Object_IsObjectType(object_id, g_EnemyObjects)
        || Object_IsObjectType(object_id, g_AllyObjects);
}

static COMMAND_RESULT M_KillAllEnemies(void)
{
    int32_t num_killed = 0;
    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (!Creature_IsEnemy(item)) {
            continue;
        }
        if (Lara_Cheat_KillEnemy(item_num)) {
            num_killed++;
        }
    }

    if (num_killed == 0) {
        Console_Log(GS(OSD_KILL_ALL_FAIL));
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_KILL_ALL), num_killed);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_KillNearestEnemies(void)
{
    bool found = false;
    while (true) {
        const int16_t best_item_num = Lara_GetNearestEnemy();
        if (best_item_num == NO_ITEM) {
            break;
        }

        const ITEM *const lara_item = Lara_GetItem();
        const ITEM *const item = Item_Get(best_item_num);
        const int32_t distance = Item_GetDistance(item, &lara_item->pos);
        found |= Lara_Cheat_KillEnemy(best_item_num);
        if (distance >= WALL_L) {
            break;
        }
    }

    if (!found) {
        Console_Log(GS(OSD_KILL_FAIL));
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_KILL));
    return CR_SUCCESS;
}

static COMMAND_RESULT M_KillEnemyType(const char *const enemy_name)
{
    bool matches_found = false;
    int32_t num_killed = 0;
    int32_t match_count = 0;
    GAME_OBJECT_ID *matching_objs =
        Object_IdsFromName(enemy_name, &match_count, M_CanTargetObjectCreature);

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);

        bool is_matched = false;
        for (int32_t i = 0; i < match_count; i++) {
            if (matching_objs[i] == item->object_id) {
                is_matched = true;
                break;
            }
        }
        if (!is_matched) {
            continue;
        }
        matches_found = true;

        if (Lara_Cheat_KillEnemy(item_num)) {
            num_killed++;
        }
    }
    Memory_FreePointer(&matching_objs);

    if (!matches_found) {
        Console_Log(GS(OSD_INVALID_OBJECT), enemy_name);
        return CR_FAILURE;
    }
    if (num_killed == 0) {
        Console_Log(GS(OSD_OBJECT_NOT_FOUND), enemy_name);
        return CR_FAILURE;
    }
    Console_Log(GS(OSD_KILL_ALL), num_killed);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_Equivalent(ctx->args, "all")) {
        return M_KillAllEnemies();
    }

    if (String_IsEmpty(ctx->args)) {
        return M_KillNearestEnemies();
    }

    return M_KillEnemyType(ctx->args);
}

CONSOLE_COMMAND g_Console_Cmd_Kill = {
    .prefix = "kill",
    .proc = M_Entrypoint,
};
