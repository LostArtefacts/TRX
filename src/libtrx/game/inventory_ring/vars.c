#include "game/inventory_ring/vars.h"

const INVENTORY_ITEM *g_InvRing_Items[] = {
    // Items
    &g_InvRing_Item_SmallMedi,
    &g_InvRing_Item_LargeMedi,
#if TR_VERSION >= 2
    &g_InvRing_Item_Flare,
#endif

    // Pickups
    &g_InvRing_Item_Pickup1,
    &g_InvRing_Item_Pickup2,
    &g_InvRing_Item_Puzzle1,
    &g_InvRing_Item_Puzzle2,
    &g_InvRing_Item_Puzzle3,
    &g_InvRing_Item_Puzzle4,
    &g_InvRing_Item_Key1,
    &g_InvRing_Item_Key2,
    &g_InvRing_Item_Key3,
    &g_InvRing_Item_Key4,
#if TR_VERSION == 1
    &g_InvRing_Item_LeadBar,
    &g_InvRing_Item_Scion,
#endif

    &g_InvRing_Item_Pistols,
    &g_InvRing_Item_Shotgun,
    &g_InvRing_Item_Magnums,
    &g_InvRing_Item_Uzis,
#if TR_VERSION >= 2
    &g_InvRing_Item_Harpoon,
    &g_InvRing_Item_M16,
    &g_InvRing_Item_Grenade,
#endif

    // Ammo
    &g_InvRing_Item_PistolAmmo,
    &g_InvRing_Item_ShotgunAmmo,
    &g_InvRing_Item_MagnumAmmo,
    &g_InvRing_Item_UziAmmo,
#if TR_VERSION >= 2
    &g_InvRing_Item_HarpoonAmmo,
    &g_InvRing_Item_M16Ammo,
    &g_InvRing_Item_GrenadeAmmo,
#endif

    // Options, etc.
    &g_InvRing_Item_Passport,
    &g_InvRing_Item_Graphics,
    &g_InvRing_Item_Sound,
    &g_InvRing_Item_Controls,
    &g_InvRing_Item_Photo,
    &g_InvRing_Item_NatlasPDA,
#if TR_VERSION == 1
    &g_InvRing_Item_Compass,
#else
    &g_InvRing_Item_Stopwatch,
#endif

    nullptr,
};
