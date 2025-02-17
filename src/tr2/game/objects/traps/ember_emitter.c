#include "game/objects/traps/ember_emitter.h"

#include "game/effects.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void EmberEmitter_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos.x = item->pos.x;
        effect->pos.y = item->pos.y;
        effect->pos.z = item->pos.z;
        effect->rot.y = 2 * Random_GetControl() + 0x8000;
        effect->speed = Random_GetControl() >> 10;
        effect->fall_speed = Random_GetControl() / -200;
        effect->frame_num = (-4 * Random_GetControl()) / 0x7FFF;
        effect->object_id = O_EMBER;
        Sound_Effect(SFX_SANDBAG_HIT, &item->pos, SPM_NORMAL);
    }
}

void EmberEmitter_Setup(void)
{
    OBJECT *const obj = Object_Get(O_EMBER_EMITTER);
    obj->control_func = EmberEmitter_Control;
    obj->collision_func = Object_Collision;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}
