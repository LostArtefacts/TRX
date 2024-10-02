#include "game/objects/traps/flame_emitter.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/sound.h"

void FlameEmitter_Setup(OBJECT *obj)
{
    obj->control = FlameEmitter_Control;
    obj->draw_routine = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void FlameEmitter_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        if (!item->data) {
            int16_t fx_num = Effect_Create(item->room_num);
            if (fx_num != NO_ITEM) {
                FX *fx = &g_Effects[fx_num];
                fx->pos.x = item->pos.x;
                fx->pos.y = item->pos.y;
                fx->pos.z = item->pos.z;
                fx->frame_num = 0;
                fx->object_id = O_FLAME;
                fx->counter = 0;
            }
            item->data = (void *)(intptr_t)(fx_num + 1);
        }
    } else if (item->data) {
        Sound_StopEffect(SFX_FIRE, NULL);
        Effect_Kill((int16_t)(intptr_t)item->data - 1);
        item->data = NULL;
    }
}
