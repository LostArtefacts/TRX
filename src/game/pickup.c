#include "game/lara.h"
#include "game/pickup.h"
#include "game/vars.h"
#include "config.h"

void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal)
{
    lara_item->goal_anim_state = goal;
    do {
        AnimateLara(lara_item);
    } while (lara_item->current_anim_state != goal);
}

int32_t KeyTrigger(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status == IS_ACTIVE
        && (T1MConfig.fix_key_triggers ? Lara.gun_status != LGS_HANDSBUSY
                                       : Lara.gun_status == LGS_ARMLESS)) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
    return 0;
}

int32_t PickupTrigger(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status != IS_INVISIBLE) {
        return 0;
    }
    item->status = IS_DEACTIVATED;
    return 1;
}

void T1MInjectGamePickup()
{
    INJECT(0x00433B40, PuzzleHoleCollision);
    INJECT(0x00433EA0, KeyTrigger);
    INJECT(0x00433EF0, PickupTrigger);
}
