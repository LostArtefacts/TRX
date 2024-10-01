#include "game/objects/traps/ember_emitter.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void EmberEmitter_Setup(OBJECT *obj)
{
    obj->control = EmberEmitter_Control;
    obj->draw_routine = Object_DrawDummyItem;
    obj->collision = Object_Collision;
    obj->save_flags = 1;
}

void EmberEmitter_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    int16_t fx_num = Effect_Create(item->room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->pos.x = item->pos.x;
        fx->pos.y = item->pos.y;
        fx->pos.z = item->pos.z;
        fx->rot.y = (Random_GetControl() - 0x4000) * 2;
        fx->speed = Random_GetControl() >> 10;
        fx->fall_speed = -Random_GetControl() / 200;
        fx->frame_num = -4 * Random_GetControl() / 0x7FFF;
        fx->object_id = O_EMBER;
        Sound_Effect(SFX_LAVA_FOUNTAIN, &item->pos, SPM_NORMAL);
    }
}
