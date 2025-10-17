#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/const.h"
#include "game/creature.h"
#include "game/game.h"
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

#if TR_VERSION == 2
extern bool CombatEnd_IsWaitingForBoss(void);
extern OBJECT_ID CombatEnd_GetBossType(void);
#endif

static bool M_CanTargetObjectCreature(const OBJECT_ID obj_id)
{
    return (Object_IsType(obj_id, g_EnemyObjects)
            || Object_IsType(obj_id, g_AllyObjects)
            || Object_IsType(obj_id, g_LoyalObjects))
        && Object_Get(obj_id)->loaded;
}

static bool M_KillSingleEnemyInRange(const int32_t max_dist)
{
    const ITEM *const lara_item = Lara_GetItem();
    int32_t best_dist = -1;
    int16_t best_item_num = NO_ITEM;
    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (Creature_IsHostile(item)) {
            const int32_t dist = Item_GetDistance(item, &lara_item->pos);
            if (dist <= max_dist) {
                if (best_item_num == NO_ITEM || dist < best_dist) {
                    best_dist = dist;
                    best_item_num = item_num;
                }
            }
        }
    }
    if (best_item_num != NO_ITEM) {
        if (Lara_Cheat_KillEnemy(best_item_num)) {
            return true;
        }
    }
    return false;
}

static int32_t M_KillAllEnemiesInRange(const int32_t max_dist)
{
    int32_t kill_count = 0;
    const ITEM *const lara_item = Lara_GetItem();
    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (Creature_IsHostile(item)) {
            const int32_t dist = Item_GetDistance(item, &lara_item->pos);
            if (dist <= max_dist) {
                // Kill this enemy
                if (Lara_Cheat_KillEnemy(item_num)) {
                    kill_count++;
                }
            }
        }
    }
    return kill_count;
}

static COMMAND_RESULT M_KillAllEnemies(void)
{
    int32_t num_killed = 0;

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (!Creature_IsHostile(item)) {
            continue;
        }
#if TR_VERSION == 2
        if (item->object_id == CombatEnd_GetBossType()
            && CombatEnd_IsWaitingForBoss()) {
            continue;
        }
#endif
        if (Lara_Cheat_KillEnemy(item_num)) {
            num_killed++;
        }
    }

    if (num_killed == 0) {
        Console_LogError(GS(OSD_KILL_ALL_FAIL));
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_KILL_ALL), num_killed);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_KillNearestEnemies(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    int32_t kill_count = M_KillAllEnemiesInRange(WALL_L);
    if (kill_count == 0) {
        kill_count = M_KillSingleEnemyInRange(5 * WALL_L);
    }

    if (kill_count == 0) {
        // No enemies killed
        Console_LogError(GS(OSD_KILL_FAIL));
        return CR_FAILURE;
    } else {
        // At least one enemy was killed.
        Console_Log(GS(OSD_KILL));
        return CR_SUCCESS;
    }
}

static COMMAND_RESULT M_KillEnemyType(const char *const enemy_name)
{
    bool matches_found = false;
    int32_t num_killed = 0;
    int32_t match_count = 0;
    OBJECT_NAME_MATCH *matches =
        Object_IdsFromName(enemy_name, &match_count, M_CanTargetObjectCreature);

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);

        bool is_matched = false;
        for (int32_t i = 0; i < match_count; i++) {
            if (matches[i].object_id == item->object_id) {
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
    Memory_FreePointer(&matches);

    if (!matches_found) {
        Console_LogError(GS(OSD_INVALID_OBJECT), enemy_name);
        return CR_FAILURE;
    }
    if (num_killed == 0) {
        Console_LogError(GS(OSD_OBJECT_NOT_FOUND), enemy_name);
        return CR_FAILURE;
    }
    Console_Log(GS(OSD_KILL_ALL), num_killed);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsLoaded()) {
        return CR_UNAVAILABLE;
    }

    if (String_Equivalent(ctx->args, "all")) {
        return M_KillAllEnemies();
    }

    if (String_IsEmpty(ctx->args)) {
        return M_KillNearestEnemies();
    }

    return M_KillEnemyType(ctx->args);
}

REGISTER_CONSOLE_COMMAND("kill", M_Entrypoint, GS_ID(CONSOLE_HELP_KILL))
