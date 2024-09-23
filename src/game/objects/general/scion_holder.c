#include "game/objects/general/scion_holder.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "global/vars.h"

void ScionHolder_Setup(OBJECT *obj)
{
    obj->control = ScionHolder_Control;
    obj->collision = Object_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void ScionHolder_Control(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}
