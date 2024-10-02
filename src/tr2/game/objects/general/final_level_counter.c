#include "game/objects/general/final_level_counter.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/los.h"
#include "game/lot.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define CUTSCENE_DELAY (5 * FRAMES_PER_SECOND) // = 150

static int16_t __cdecl M_FindBestBoss(void);
static void __cdecl M_ActivateLastBoss(void);
static void __cdecl M_PrepareCutscene(int16_t item_num);

static int16_t __cdecl M_FindBestBoss(void)
{
    int32_t best_dist = 0;
    int16_t best_item = g_FinalBossItem[0];
    for (int32_t i = 0; i < g_FinalBossCount; i++) {
        const ITEM *const item = &g_Items[g_FinalBossItem[i]];

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

        g_LaraItem = g_LaraItem;
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

static void __cdecl M_ActivateLastBoss(void)
{
    const int16_t item_num = M_FindBestBoss();
    ITEM *const item = &g_Items[item_num];
    item->touch_bits = 0;
    item->status = IS_ACTIVE;
    item->mesh_bits = 0xFFFF1FFF;
    Item_AddActive(item_num);
    LOT_EnableBaddieAI(item_num, true);
    g_FinalBossActive = 1;
}

static void __cdecl M_PrepareCutscene(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    Creature_Kill(item, 0, 0, LA_EXTRA_FINAL_ANIM);

    g_Camera.type = CAM_CINEMATIC;
    g_Lara.mesh_ptrs[LM_HAND_R] =
        g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HAND_R];
    g_CineFrameIdx = 428;
    g_CinePos.pos = item->pos;
    g_CinePos.rot = item->rot;
}

void __cdecl FinalLevelCounter_Control(const int16_t item_num)
{
    if (g_SaveGame.statistics.kills == g_FinalLevelCount
        && !g_FinalBossActive) {
        M_ActivateLastBoss();
        return;
    }

    if (g_SaveGame.statistics.kills > g_FinalLevelCount) {
        g_FinalBossActive++;
        if (g_FinalBossActive == CUTSCENE_DELAY) {
            M_PrepareCutscene(item_num);
        }
    }
}
