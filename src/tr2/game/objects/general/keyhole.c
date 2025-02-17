#include "game/objects/general/keyhole.h"

#include "game/game_flow.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/sound.h"
#include "global/vars.h"

static XYZ_32 m_KeyholePosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - LARA_RADIUS - 50,
};

static int16_t m_KeyholeBounds[12] = {
    // clang-format off
    -200,
    +200,
    +0,
    +0,
    +WALL_L / 2 - 200,
    +WALL_L / 2,
    -10 * DEG_1,
    +10 * DEG_1,
    -30 * DEG_1,
    +30 * DEG_1,
    -10 * DEG_1,
    +10 * DEG_1,
    // clang-format on
};

static void M_Consume(
    ITEM *lara_item, ITEM *keyhole_item, GAME_OBJECT_ID key_obj_id);
static void M_Refuse(const ITEM *lara_item);

static void M_Refuse(const ITEM *const lara_item)
{
    if (lara_item->pos.x == g_InteractPosition.x
        && lara_item->pos.y == g_InteractPosition.y
        && lara_item->pos.z == g_InteractPosition.z) {
        return;
    }

    Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_NORMAL);
    g_InteractPosition = lara_item->pos;
}

static void M_Consume(
    ITEM *const lara_item, ITEM *const keyhole_item,
    const GAME_OBJECT_ID key_obj_id)
{
    Inv_RemoveItem(key_obj_id);
    Item_AlignPosition(&m_KeyholePosition, keyhole_item, lara_item);
    lara_item->goal_anim_state = LS_USE_KEY;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_USE_KEY);
    lara_item->goal_anim_state = LS_STOP;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    keyhole_item->status = IS_ACTIVE;
    g_InteractPosition = lara_item->pos;
}

void Keyhole_Setup(OBJECT *const obj)
{
    obj->collision_func = Keyhole_Collision;
    obj->save_flags = 1;
}

void Keyhole_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if ((g_Inv_Chosen == NO_OBJECT && !g_Input.action)
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity) {
        return;
    }

    if (!Item_TestPosition(m_KeyholeBounds, item, lara_item)) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        M_Refuse(lara_item);
        return;
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        GF_ShowInventoryKeys(item->object_id);
        if (g_Inv_Chosen == NO_OBJECT && g_InvRing_Source[RT_KEYS].count > 0) {
            return;
        }
    }
    if (g_Inv_Chosen != NO_OBJECT) {
        g_InteractPosition.y = lara_item->pos.y - 1;
    }

    const GAME_OBJECT_ID key_obj_id =
        Object_GetCognateInverse(item->object_id, g_KeyItemToReceptacleMap);
    const bool correct = g_Inv_Chosen == key_obj_id;
    g_Inv_Chosen = NO_OBJECT;

    if (correct) {
        M_Consume(lara_item, item, key_obj_id);
    } else {
        M_Refuse(lara_item);
    }
}

int32_t Keyhole_Trigger(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_ACTIVE || g_Lara.gun_status == LGS_HANDS_BUSY) {
        return false;
    }
    item->status = IS_DEACTIVATED;
    return true;
}
