#include "game/item_actions/bubbles.h"

#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/game/collision.h>

void ItemAction_Bubbles(ITEM *item)
{
    // XXX: until we get RoboLara, it makes sense for her to breathe underwater
    if (g_Lara.water_status == LWS_CHEAT
        && !(Room_Get(g_LaraItem->room_num)->flags & RF_UNDERWATER)) {
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
