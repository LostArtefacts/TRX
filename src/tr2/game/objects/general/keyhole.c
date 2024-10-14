#include "game/objects/general/keyhole.h"

#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/inventory/common.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/sound.h"
#include "global/vars.h"

static void M_Consume(
    ITEM *lara_item, ITEM *keyhole_item, GAME_OBJECT_ID key_object_id);
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
    const GAME_OBJECT_ID key_object_id)
{
    Inv_RemoveItem(key_object_id);
    Item_AlignPosition(&g_KeyholePosition, keyhole_item, lara_item);
    lara_item->goal_anim_state = LS_USE_KEY;
    do {
        Lara_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_USE_KEY);
    lara_item->goal_anim_state = LS_STOP;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    keyhole_item->status = IS_ACTIVE;
    g_InteractPosition = lara_item->pos;
}

void __cdecl Keyhole_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->current_anim_state != LS_STOP) {
        return;
    }

    ITEM *const item = &g_Items[item_num];
    if ((g_Inv_Chosen == NO_OBJECT && !(g_Input & IN_ACTION))
        || g_Lara.gun_status != LGS_ARMLESS || lara_item->gravity) {
        return;
    }

    if (!Item_TestPosition(g_KeyholeBounds, &g_Items[item_num], lara_item)) {
        return;
    }

    if (item->status != IS_INACTIVE) {
        M_Refuse(lara_item);
        return;
    }

    if (g_Inv_Chosen == NO_OBJECT) {
        Inv_Display(INV_KEYS_MODE);
        if (g_Inv_Chosen == NO_OBJECT && g_Inv_KeyObjectsCount > 0) {
            return;
        }
    }
    if (g_Inv_Chosen != NO_OBJECT) {
        g_InteractPosition.y = lara_item->pos.y - 1;
    }

    const GAME_OBJECT_ID key_object_id =
        Object_GetCognateInverse(item->object_id, g_KeyItemToReceptacleMap);
    const bool correct = g_Inv_Chosen == key_object_id;
    g_Inv_Chosen = NO_OBJECT;

    if (correct) {
        M_Consume(lara_item, item, key_object_id);
    } else {
        M_Refuse(lara_item);
    }
}

int32_t __cdecl Keyhole_Trigger(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    if (item->status != IS_ACTIVE || g_Lara.gun_status == LGS_HANDS_BUSY) {
        return false;
    }
    item->status = IS_DEACTIVATED;
    return true;
}
