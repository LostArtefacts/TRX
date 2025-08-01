#pragma once

#include "./types.h"

extern INV_RING_SOURCE g_InvRing_Source[RT_NUMBER_OF];
extern const INVENTORY_ITEM *g_InvRing_Items[];

// Items
extern INVENTORY_ITEM g_InvRing_Item_SmallMedi;
extern INVENTORY_ITEM g_InvRing_Item_LargeMedi;
#if TR_VERSION >= 2
extern INVENTORY_ITEM g_InvRing_Item_Flare;
#endif

// Pickups
extern INVENTORY_ITEM g_InvRing_Item_Pickup1;
extern INVENTORY_ITEM g_InvRing_Item_Pickup2;
extern INVENTORY_ITEM g_InvRing_Item_Puzzle1;
extern INVENTORY_ITEM g_InvRing_Item_Puzzle2;
extern INVENTORY_ITEM g_InvRing_Item_Puzzle3;
extern INVENTORY_ITEM g_InvRing_Item_Puzzle4;
extern INVENTORY_ITEM g_InvRing_Item_Key1;
extern INVENTORY_ITEM g_InvRing_Item_Key2;
extern INVENTORY_ITEM g_InvRing_Item_Key3;
extern INVENTORY_ITEM g_InvRing_Item_Key4;
#if TR_VERSION == 1
extern INVENTORY_ITEM g_InvRing_Item_LeadBar;
extern INVENTORY_ITEM g_InvRing_Item_Scion;
#endif

// Weapons
extern INVENTORY_ITEM g_InvRing_Item_Pistols;
extern INVENTORY_ITEM g_InvRing_Item_Shotgun;
extern INVENTORY_ITEM g_InvRing_Item_Magnums;
extern INVENTORY_ITEM g_InvRing_Item_Uzis;
#if TR_VERSION >= 2
extern INVENTORY_ITEM g_InvRing_Item_Harpoon;
extern INVENTORY_ITEM g_InvRing_Item_M16;
extern INVENTORY_ITEM g_InvRing_Item_Grenade;
#endif

// Ammo
extern INVENTORY_ITEM g_InvRing_Item_PistolAmmo;
extern INVENTORY_ITEM g_InvRing_Item_ShotgunAmmo;
extern INVENTORY_ITEM g_InvRing_Item_MagnumAmmo;
extern INVENTORY_ITEM g_InvRing_Item_UziAmmo;
#if TR_VERSION >= 2
extern INVENTORY_ITEM g_InvRing_Item_HarpoonAmmo;
extern INVENTORY_ITEM g_InvRing_Item_M16Ammo;
extern INVENTORY_ITEM g_InvRing_Item_GrenadeAmmo;
#endif

// Options, etc.
#if TR_VERSION == 1
extern INVENTORY_ITEM g_InvRing_Item_Compass;
#else
extern INVENTORY_ITEM g_InvRing_Item_Stopwatch;
#endif
extern INVENTORY_ITEM g_InvRing_Item_Passport;
extern INVENTORY_ITEM g_InvRing_Item_Graphics;
extern INVENTORY_ITEM g_InvRing_Item_Sound;
extern INVENTORY_ITEM g_InvRing_Item_Controls;
extern INVENTORY_ITEM g_InvRing_Item_Photo;
extern INVENTORY_ITEM g_InvRing_Item_NatlasPDA;
