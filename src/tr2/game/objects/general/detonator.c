#include "game/objects/general/detonator.h"

#include "game/items.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/vars.h"

void __cdecl Detonator_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);

    const int32_t frame_num =
        item->frame_num - g_Anims[item->anim_num].frame_base;
    if (frame_num > 75 && frame_num < 100) {
        AddDynamicLight(item->pos.x, item->pos.y, item->pos.z, 13, 11);
    }

    if (frame_num == 80) {
        g_Camera.bounce = -150;
        Sound_Effect(SFX_EXPLOSION1, NULL, SPM_ALWAYS);
    }

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}
