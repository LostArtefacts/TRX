#include "game/items.h"
#include "global/vars.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (g_FinalBossActive >= 5 * FRAMES_PER_SECOND) {
        item->status = IS_ACTIVE;
        Item_Animate(item);
    } else {
        item->status = IS_INVISIBLE;
    }
}

REGISTER_OBJECT(O_CUT_SHOTGUN, M_Setup)
