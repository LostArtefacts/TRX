#pragma once

#include <trx/game/lara/skin/types.h>

// The outfit Lara wears in gold. It is the outfit itself, drawing from gilded
// twins of the objects it names, so gilding her leaves every other item those
// objects dress - the Bacon Lara doppelganger among them - as it was. Returns
// nullptr where the level carries nothing to gild.
const LARA_SKIN_OUTFIT *Lara_Skin_GetGoldOutfit(const LARA_SKIN_OUTFIT *outfit);

// The gilded twin of a lone mesh, for one Lara wears that no outfit object
// holds - the torso a level puts on young Lara before she has her backpack.
OBJECT_MESH *Lara_Skin_GetGoldMesh(OBJECT_MESH *mesh, RGB_888 color);

void Lara_Skin_ResetGold(void);
