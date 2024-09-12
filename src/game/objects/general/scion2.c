#include "game/objects/general/scion2.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/objects/general/pickup.h"
#include "global/vars.h"

void Scion2_Setup(OBJECT_INFO *obj)
{
    g_Objects[O_SCION_ITEM2].draw_routine = Object_DrawPickupItem;
    g_Objects[O_SCION_ITEM2].collision = Pickup_Collision;
    g_Objects[O_SCION_ITEM2].save_flags = 1;
    g_Objects[O_SCION_ITEM2].bounds = Pickup_Bounds;
}
