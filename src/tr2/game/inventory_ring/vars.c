#include "game/inventory_ring/vars.h"

#include "global/vars.h"

int32_t g_Inv_NFrames = 2;

INV_RING_SOURCE g_InvRing_Source[RT_NUMBER_OF] = {
    [RT_KEYS] = {
        .count = 0,
        .current = 0,
        .qtys = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        .items = {
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
        .count = 8,
        .current = 0,
        .qtys = { 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        .items = {
            &g_InvRing_Item_Stopwatch,
            &g_InvRing_Item_Flare,
            &g_InvRing_Item_Pistols,
            &g_InvRing_Item_Shotgun,
            &g_InvRing_Item_Magnums,
            &g_InvRing_Item_Uzis,
            &g_InvRing_Item_M16,
            &g_InvRing_Item_Grenade,
            &g_InvRing_Item_Harpoon,
            &g_InvRing_Item_LargeMedi,
            &g_InvRing_Item_SmallMedi,
        },
    },
    [RT_OPTION] = {
        .count = 6,
        .current = 0,
        .qtys = { 1, 1, 1, 1, 1, 1 },
        .items = {
            &g_InvRing_Item_Passport,
            &g_InvRing_Item_Graphics,
            &g_InvRing_Item_Controls,
            &g_InvRing_Item_Sound,
            &g_InvRing_Item_NatlasPDA,
            &g_InvRing_Item_Photo,
        },
    },
};
