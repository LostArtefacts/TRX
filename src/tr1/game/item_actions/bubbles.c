#include "game/item_actions/bubbles.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/random.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/spawn.h>

void ItemAction_Bubbles(ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();

    // XXX: until we get RoboLara, it makes sense for her to breathe underwater
    if (lara->water_status == LWS_CHEAT
        && !(Room_Get(lara_item->room_num)->flags & RF_UNDERWATER)) {
        return;
    }

    int32_t count = (Random_GetDraw() * 3) / 0x8000;
    if (!count) {
        return;
    }

    Sound_Effect(SFX_LARA_BUBBLES, &item->pos, SPM_UNDERWATER);

    XYZ_32 offset = { .x = 0, .y = 0, .z = 50 };
    Collide_GetJointAbsPosition(item, &offset, LM_HEAD);

    for (int32_t i = 0; i < count; i++) {
        Spawn_Bubble(&offset, item->room_num);
    }
}
