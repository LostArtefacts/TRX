#include "game/objects/traps/flame_emitter.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "global/vars.h"

void FlameEmitter_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        if (item->data != nullptr) {
            const int32_t flame_num = ((int32_t)(intptr_t)item->data) - 1;
            Effect_Kill(flame_num);
            item->data = nullptr;
        }
    } else if (item->data == nullptr) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos.x = item->pos.x;
            effect->pos.y = item->pos.y;
            effect->pos.z = item->pos.z;
            effect->frame_num = 0;
            effect->object_id = O_FLAME;
            effect->counter = 0;
        }
        item->data = (void *)(intptr_t)(effect_num + 1);
    }
}

void FlameEmitter_Setup(void)
{
    OBJECT *const obj = Object_Get(O_FLAME_EMITTER);
    obj->control_func = FlameEmitter_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}
