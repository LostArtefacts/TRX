#pragma once

#include <trx/game/lara/skin/types.h>

void Lara_Skin_LoadFromFile(const char *path);
void Lara_Skin_Shutdown(void);
int32_t Lara_Skin_GetOutfitCount(void);
int32_t Lara_Skin_GetBraidCount(void);
bool Lara_Skin_IsOutfitAvailable(LARA_SKIN_TYPE skin_type);
const LARA_SKIN_OUTFIT *Lara_Skin_GetOutfit(LARA_SKIN_TYPE skin_type);
const LARA_SKIN_BRAID *Lara_Skin_GetBraid(int32_t idx);
int32_t Lara_Skin_GetExtraMeshOffset(LARA_SKIN_EXTRA_MESH mesh);
