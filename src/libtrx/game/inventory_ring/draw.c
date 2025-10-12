#include "game/inventory_ring/draw.h"

#include "game/inventory_ring/vars.h"

const INVENTORY_ITEM *InvRing_GetInvItem(const OBJECT_ID obj_id)
{
    for (int32_t i = 0; g_InvRing_Items[i] != nullptr; i++) {
        if (g_InvRing_Items[i]->object_id == obj_id) {
            return g_InvRing_Items[i];
        }
    }
    return nullptr;
}
