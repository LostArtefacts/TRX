#include "game/console/cmd/kill.h"

#include "game/game_string.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_cheat.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/game/objects/ids.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

static bool M_CanTargetObject_Enemy(GAME_OBJECT_ID object_id);
static COMMAND_RESULT M_Entrypoint(const char *args);

static bool M_CanTargetObject_Enemy(const GAME_OBJECT_ID object_id)
{
    return Object_IsObjectType(object_id, g_EnemyObjects);
}

static COMMAND_RESULT M_Entrypoint(const char *args)
{
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

    // kill all the enemies in the level
    if (String_Equivalent(args, "all")) {
        int32_t num = 0;
        for (int16_t item_num = 0; item_num < Item_GetTotalCount();
             item_num++) {
            if (Lara_Cheat_KillEnemy(item_num)) {
                num++;
            }
        }

        if (num == 0) {
            Console_Log(GS(OSD_KILL_ALL_FAIL), num);
            return CR_FAILURE;
        }

        Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
        Console_Log(GS(OSD_KILL_ALL), num);
        return CR_SUCCESS;
    }

    // kill all the enemies around Lara within one tile, or a single nearest
    // enemy
    if (String_Equivalent(args, "")) {
        bool found = false;
        while (true) {
            const int16_t best_item_num = Lara_GetNearestEnemy();
            if (best_item_num == NO_ITEM) {
                break;
            }

            ITEM_INFO *const item = &g_Items[best_item_num];
            const int32_t distance = Item_GetDistance(item, &g_LaraItem->pos);
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

    // kill a single enemy type
    {
        int32_t match_count = 0;
        GAME_OBJECT_ID *matching_objs =
            Object_IdsFromName(args, &match_count, M_CanTargetObject_Enemy);
        int32_t num = 0;
        for (int32_t i = 0; i < match_count; i++) {
            const GAME_OBJECT_ID object_id = matching_objs[i];
            for (int16_t item_num = 0; item_num < Item_GetTotalCount();
                 item_num++) {
                if (g_Items[item_num].object_id == object_id
                    && Lara_Cheat_KillEnemy(item_num)) {
                    num++;
                }
            }
        }
        Memory_FreePointer(&matching_objs);

        if (!match_count) {
            return CR_BAD_INVOCATION;
        }
        if (num == 0) {
            Console_Log(GS(OSD_KILL_ALL_FAIL));
            return CR_FAILURE;
        }
        Console_Log(GS(OSD_KILL_ALL), num);
        return CR_SUCCESS;
    }
}

CONSOLE_COMMAND g_Console_Cmd_Kill = {
    .prefix = "kill",
    .proc = M_Entrypoint,
};
