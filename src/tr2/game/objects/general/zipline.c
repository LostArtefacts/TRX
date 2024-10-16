#include "game/objects/general/zipline.h"

#include "game/input.h"
#include "game/items.h"
#include "global/vars.h"

void __cdecl Zipline_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!(g_Input & IN_ACTION) || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity || lara_item->current_anim_state != LS_STOP) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INACTIVE) {
        return;
    }

    if (!Item_TestPosition(g_ZiplineHandleBounds, item, lara_item)) {
        return;
    }

    Item_AlignPosition(&g_ZiplineHandlePosition, item, lara_item);
    g_Lara.gun_status = LGS_HANDS_BUSY;

    lara_item->goal_anim_state = LS_ZIPLINE;
    do {
        Item_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_NULL);

    if (!item->active) {
        Item_AddActive(item_num);
    }

    item->status = IS_ACTIVE;
    item->flags |= IF_ONE_SHOT;
}
