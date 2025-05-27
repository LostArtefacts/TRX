#include "game/objects/general/combat_end.h"

#include <libtrx/game/objects.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (CombatEnd_IsComplete()) {
        item->status = IS_ACTIVE;
        Item_Animate(item);
    } else {
        item->status = IS_INVISIBLE;
    }
}

REGISTER_OBJECT(O_CUT_SHOTGUN, M_Setup)
