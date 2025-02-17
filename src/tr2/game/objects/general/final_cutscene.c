#include "game/objects/general/final_cutscene.h"

#include "game/items.h"
#include "global/vars.h"

void FinalCutscene_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (g_FinalBossActive >= 5 * FRAMES_PER_SECOND) {
        item->status = IS_ACTIVE;
        Item_Animate(item);
    } else {
        item->status = IS_INVISIBLE;
    }
}

void FinalCutscene_Setup(void)
{
    OBJECT *const obj = Object_Get(O_CUT_SHOTGUN);
    obj->control_func = FinalCutscene_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
