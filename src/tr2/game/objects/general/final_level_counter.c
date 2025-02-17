#include "game/objects/general/final_level_counter.h"

#include "decomp/flares.h"
#include "game/camera.h"
#include "game/creature.h"
#include "game/gun/gun.h"
#include "game/items.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/objects/common.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define CUTSCENE_DELAY (5 * FRAMES_PER_SECOND) // = 150

static int16_t M_FindBestBoss(void);
static void M_ActivateLastBoss(void);
static void M_PrepareCutscene(int16_t item_num);

static int16_t M_FindBestBoss(void)
{
    int32_t best_dist = 0;
    int16_t best_item = g_FinalBossItem[0];
    for (int32_t i = 0; i < g_FinalBossCount; i++) {
        const ITEM *const item = Item_Get(g_FinalBossItem[i]);

        GAME_VECTOR start;
        start.pos.x = g_LaraItem->pos.x;
        start.pos.y = g_LaraItem->pos.y - STEP_L * 2;
        start.pos.z = g_LaraItem->pos.z;
        start.room_num = g_LaraItem->room_num;

        GAME_VECTOR target;
        target.pos.x = item->pos.x;
        target.pos.y = item->pos.y - STEP_L * 2;
        target.pos.z = item->pos.z;
        target.room_num = item->room_num;

        if (!LOS_Check(&start, &target)) {
            const int32_t dx = (g_LaraItem->pos.x - item->pos.x) >> 6;
            const int32_t dy = (g_LaraItem->pos.y - item->pos.y) >> 6;
            const int32_t dz = (g_LaraItem->pos.z - item->pos.z) >> 6;
            const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
            if (dist < best_dist) {
                best_dist = dist;
                best_item = g_FinalBossItem[i];
            }
        }
    }
    return best_item;
}

static void M_ActivateLastBoss(void)
{
    const int16_t item_num = M_FindBestBoss();
    ITEM *const item = Item_Get(item_num);
    item->touch_bits = 0;
    item->status = IS_ACTIVE;
    item->mesh_bits = 0xFFFF1FFF;
    Item_AddActive(item_num);
    LOT_EnableBaddieAI(item_num, true);
    g_FinalBossActive = 1;
}

static void M_PrepareCutscene(const int16_t item_num)
{
    if (g_Lara.gun_type == LGT_FLARE) {
        Flare_Undraw();
        g_Lara.flare_control_left = false;
        g_Lara.left_arm.lock = false;
    }

    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    g_Lara.water_status = LWS_ABOVE_WATER;

    ITEM *const item = Item_Get(item_num);
    Creature_Kill(item, 0, 0, LA_EXTRA_FINAL_ANIM);

    Camera_InvokeCinematic(item, 428, 0);
}

void FinalLevelCounter_Setup(void)
{
    OBJECT *const obj = Object_Get(O_FINAL_LEVEL_COUNTER);
    obj->control_func = FinalLevelCounter_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void FinalLevelCounter_Control(const int16_t item_num)
{
    if (g_SaveGame.current_stats.kills == g_FinalLevelCount
        && !g_FinalBossActive) {
        M_ActivateLastBoss();
        return;
    }

    if (g_SaveGame.current_stats.kills > g_FinalLevelCount) {
        g_FinalBossActive++;
        if (g_FinalBossActive == CUTSCENE_DELAY) {
            M_PrepareCutscene(item_num);
        }
    }
}
