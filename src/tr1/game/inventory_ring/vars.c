#include "game/inventory_ring/vars.h"

#include <stdint.h>

INV_RING_SOURCE g_InvRing_Source[RT_NUMBER_OF] = {
    [RT_KEYS] = {
        .current = 0,
        .count = 0,
        .qtys = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        .items = {
            &g_InvRing_Item_LeadBar,
            &g_InvRing_Item_Puzzle1,
            &g_InvRing_Item_Puzzle2,
            &g_InvRing_Item_Puzzle3,
            &g_InvRing_Item_Puzzle4,
            &g_InvRing_Item_Key1,
            &g_InvRing_Item_Key2,
            &g_InvRing_Item_Key3,
            &g_InvRing_Item_Key4,
            &g_InvRing_Item_Pickup1,
            &g_InvRing_Item_Pickup2,
        },
    },
    [RT_MAIN] = {
        .current = 0,
        .count = 1,
        .qtys = { 1, 1, 1, 1, 1, 1, 1, 1 },
        .items = {
            &g_InvRing_Item_Compass,
            &g_InvRing_Item_Flare,
            &g_InvRing_Item_Pistols,
            &g_InvRing_Item_Shotgun,
            &g_InvRing_Item_Magnums,
            &g_InvRing_Item_Uzis,
            &g_InvRing_Item_LargeMedi,
            &g_InvRing_Item_SmallMedi,
        },
    },
    [RT_OPTION] = {
        .current = 0,
        .count = 6,
        .qtys = { 1, 1, 1, 1, 1, 1 },
        .items = {
            &g_InvRing_Item_Passport,
            &g_InvRing_Item_Controls,
            &g_InvRing_Item_Sound,
            &g_InvRing_Item_Graphics,
            &g_InvRing_Item_NatlasPDA,
            &g_InvRing_Item_Photo,
        },
    },
};
