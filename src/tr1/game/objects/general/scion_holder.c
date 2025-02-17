#include "game/items.h"
#include "game/objects/common.h"
#include "global/vars.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    Item_Animate(Item_Get(item_num));
}

REGISTER_OBJECT(O_SCION_HOLDER, M_Setup)
