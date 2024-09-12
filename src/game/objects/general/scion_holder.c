#include "game/objects/general/scion_holder.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "global/vars.h"

void ScionHolder_Setup(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_HOLDER].control = ScionHolder_Control;
    g_Objects[O_SCION_HOLDER].collision = Object_Collision;
    g_Objects[O_SCION_HOLDER].save_anim = 1;
    g_Objects[O_SCION_HOLDER].save_flags = 1;
}

void ScionHolder_Control(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}
