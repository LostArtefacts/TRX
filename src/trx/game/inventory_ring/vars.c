#include <trx/game/inventory_ring/vars.h>

#include <trx/core/memory.h>
#include <trx/core/subsystem.h>

INVENTORY_MODE g_InvRing_Mode = INV_TITLE_MODE;
CAMERA_INFO g_InvRing_OldCamera = {};
VECTOR *g_InvRing_Items = nullptr;
INV_RING_SOURCE g_InvRing_Source[RT_NUMBER_OF] = {};

static void M_Init(void)
{
    g_InvRing_Items = Vector_Create(sizeof(INVENTORY_ITEM *));
}

static void M_Shutdown(void)
{
    if (g_InvRing_Items != nullptr) {
        for (int32_t i = 0; i < g_InvRing_Items->count; i++) {
            INVENTORY_ITEM *const item =
                *(INVENTORY_ITEM **)Vector_Get(g_InvRing_Items, i);
            Memory_Free(item);
        }
        Vector_Free(g_InvRing_Items);
        g_InvRing_Items = nullptr;
    }
    // The rings point at the entries and last longer than they do, because a
    // mod switch restarts the game in place.
    for (int32_t i = 0; i < RT_NUMBER_OF; i++) {
        g_InvRing_Source[i] = (INV_RING_SOURCE) {};
    }
}

REGISTER_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
