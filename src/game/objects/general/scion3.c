#include "game/objects/general/scion3.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

void Scion3_Setup(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM3].control = Scion3_Control;
    g_Objects[O_SCION_ITEM3].hit_points = 5;
    g_Objects[O_SCION_ITEM3].save_flags = 1;
}

void Scion3_Control(int16_t item_num)
{
    static int32_t counter = 0;
    ITEM_INFO *item = &g_Items[item_num];

    if (item->hit_points > 0) {
        counter = 0;
        Item_Animate(item);
        return;
    }

    if (counter == 0) {
        item->status = IS_INVISIBLE;
        item->hit_points = DONT_TARGET;
        Room_TestTriggers(item);
        Item_RemoveDrawn(item_num);
    }

    if (counter % 10 == 0) {
        int16_t fx_num = Effect_Create(item->room_num);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = item->pos.x + (Random_GetControl() - 0x4000) / 32;
            fx->pos.y =
                item->pos.y + (Random_GetControl() - 0x4000) / 256 - 500;
            fx->pos.z = item->pos.z + (Random_GetControl() - 0x4000) / 32;
            fx->speed = 0;
            fx->frame_num = 0;
            fx->object_id = O_EXPLOSION1;
            fx->counter = 0;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
            g_Camera.bounce = -200;
        }
    }

    counter++;
    if (counter >= LOGIC_FPS * 3) {
        Item_Kill(item_num);
    }
}
