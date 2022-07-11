#include "game/effect_routines/bubbles.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/objects/effects/bubble.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void FX_Bubbles(ITEM_INFO *item)
{
    // XXX: until we get Robolara, it makes sense for her to breathe underwater
    if (g_Lara.water_status == LWS_CHEAT
        && !(g_RoomInfo[g_LaraItem->room_number].flags & RF_UNDERWATER)) {
        return;
    }

    int32_t count = (Random_GetDraw() * 3) / 0x8000;
    if (!count) {
        return;
    }

    Sound_Effect(SFX_LARA_BUBBLES, &item->pos, SPM_UNDERWATER);

    PHD_VECTOR offset;
    offset.x = 0;
    offset.y = 0;
    offset.z = 50;
    Collide_GetJointAbsPosition(item, &offset, LM_HEAD);

    for (int i = 0; i < count; i++) {
        int16_t fx_num = Effect_Create(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = offset.x;
            fx->pos.y = offset.y;
            fx->pos.z = offset.z;
            fx->object_number = O_BUBBLES1;
            fx->frame_number = -((Random_GetDraw() * 3) / 0x8000);
            fx->speed = 10 + ((Random_GetDraw() * 6) / 0x8000);
        }
    }
}
