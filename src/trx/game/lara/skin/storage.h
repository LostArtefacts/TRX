#pragma once

#include <trx/game/lara/skin/types.h>

int32_t Lara_Skin_GetOutfitCount(void);
// Whether the outfit is one the game declares, which is what a name read from
// a game flow is checked against. Say nothing about the level, which is not
// loaded when a game flow is read.
bool Lara_Skin_IsOutfitDefined(LARA_SKIN_TYPE skin_type);

// Whether the level carries the meshes to wear it.
bool Lara_Skin_IsOutfitAvailable(LARA_SKIN_TYPE skin_type);
const LARA_SKIN_OUTFIT *Lara_Skin_GetOutfit(LARA_SKIN_TYPE skin_type);
const char *Lara_Skin_GetOutfitName(LARA_SKIN_TYPE skin_type);
LARA_SKIN_TYPE Lara_Skin_FindOutfitByName(const char *name);
LARA_SKIN_TYPE Lara_Skin_GetDefaultType(void);
int32_t Lara_Skin_GetExtraMeshOffset(LARA_SKIN_EXTRA_MESH mesh);

// Return the meshes used when an outfit carries a weapon, or an empty map
// when the outfit specifies none.
LARA_SKIN_MESH_MAP Lara_Skin_GetGunMeshMap(
    const LARA_SKIN_GUN_MAP *gun_map, LARA_GUN_TYPE gun_type);
