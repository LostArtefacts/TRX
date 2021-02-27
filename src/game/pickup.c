#include "game/vars.h"
#include "game/pickup.h"
#include "config.h"

int32_t KeyTrigger(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
#ifdef T1M_FEAT_OG_FIXES
    if (item->status == IS_ACTIVE
        && (T1MConfig.fix_key_triggers ? Lara.gun_status != LGS_HANDSBUSY
                                       : Lara.gun_status == LGS_ARMLESS)) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
#else
    if (item->status == IS_ACTIVE && Lara.gun_status == LGS_ARMLESS) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
#endif
    return 0;
}

void T1MInjectGamePickup()
{
    INJECT(0x00433EA0, KeyTrigger);
}
