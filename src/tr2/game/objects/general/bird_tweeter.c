#include "game/objects/general/bird_tweeter.h"

#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"

void BirdTweeter_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);

    if (item->object_id == O_BIRD_TWEETER_2) {
        if (Random_GetDraw() < 1024) {
            Sound_Effect(SFX_BIRDS_CHIRP, &item->pos, SPM_NORMAL);
        }
    } else if (Random_GetDraw() < 256) {
        Sound_Effect(SFX_DRIPS_REVERB, &item->pos, SPM_NORMAL);
    }
}

void BirdTweeter_Setup(OBJECT *const obj)
{
    obj->control_func = BirdTweeter_Control;
    obj->draw_func = Object_DrawDummyItem;
}
