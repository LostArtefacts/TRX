#include "game/creature.h"
#include "game/game.h"
#include "game/gun/gun.h"
#include "game/lara/flare.h"
#include "game/los.h"
#include "game/objects/common.h"
#include "game/savegame.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/utils.h>

#define M_CUTSCENE_DELAY (5 * LOGIC_FPS) // = 150
#define M_BOSS_TYPE O_CULT_3

static int16_t m_BossTimer = 0;
static uint16_t m_EnemyCount = 0;

static int16_t M_FindBestBoss(void);
static void M_ActivateLastBoss(void);
static void M_PrepareCutscene(int16_t item_num);
static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);

static int16_t M_FindBestBoss(void)
{
    // Note that in the original, the first boss item was always selected here.
    // For speedruns, the change here means that is no longer guaranteed, but
    // positional manipulation can be used for the best outcome.
    int32_t best_dist = INT32_MAX;
    int16_t best_item = NO_ITEM;
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (item->object_id != M_BOSS_TYPE) {
            continue;
        }

        if (item->status == IS_ACTIVE || item->status == IS_DEACTIVATED) {
            best_item = i;
            break;
        }

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
                best_item = i;
            }
        }
    }
    return best_item;
}

static void M_ActivateLastBoss(void)
{
    const int16_t item_num = M_FindBestBoss();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_ACTIVE && item->status != IS_DEACTIVATED) {
        item->touch_bits = 0;
        item->status = IS_ACTIVE;
        item->mesh_bits = 0xFFFF1FFF;
        Item_AddActive(item_num);
        LOT_EnableBaddieAI(item_num, true);
    }
    m_BossTimer = 1;
}

static void M_PrepareCutscene(const int16_t item_num)
{
    if (g_Lara.gun_type == LGT_FLARE) {
        Lara_Flare_Undraw();
        g_Lara.flare.control = false;
        g_Lara.left_arm.lock = false;
    }

    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    g_Lara.water_status = LWS_ABOVE_WATER;
    g_Lara.target = nullptr;

    ITEM *const item = Item_Get(item_num);
    Creature_SpecialKill(item, 0, 0, LA_EXTRA_FINAL_ANIM);

    Camera_InvokeCinematic(item, 428, 0);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->initialise_func = M_Initialise;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = true;

    m_BossTimer = 0;
    m_EnemyCount = 0;
}

static void M_Initialise(const int16_t item_num)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (Object_IsType(item->object_id, g_EnemyObjects)
            && item->object_id != M_BOSS_TYPE) {
            m_EnemyCount++;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    const RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (current_info->stats.kill_count == m_EnemyCount && m_BossTimer == 0) {
        M_ActivateLastBoss();
        return;
    }

    if (current_info->stats.kill_count > m_EnemyCount) {
        m_BossTimer++;
        if (m_BossTimer == M_CUTSCENE_DELAY) {
            M_PrepareCutscene(item_num);
        }
    }
}

bool CombatEnd_IsComplete(void)
{
    return m_BossTimer >= M_CUTSCENE_DELAY;
}

REGISTER_OBJECT(O_COMBAT_END, M_Setup)
