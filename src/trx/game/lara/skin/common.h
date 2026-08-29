#pragma once

#include <trx/game/lara/enum.h>
#include <trx/game/lara/skin/types.h>
#include <trx/game/sound/ids.h>

// Drops what the outgoing level left behind, before the objects it named go.
void Lara_Skin_Reset(void);

void Lara_Skin_Initialise(void);
void Lara_Skin_ApplyOutfitFromConfig(void);
void Lara_Skin_CycleOutfit(int32_t dir);
LARA_SKIN_TYPE Lara_Skin_GetType(void);
bool Lara_Skin_IsDefaultType(void);
void Lara_Skin_SetType(LARA_SKIN_TYPE skin_type);
void Lara_Skin_ApplyOutfit(void);

// Put another mesh on one of Lara's own, in place of the one her outfit gives
// her there. It outlives an outfit change, because applying an outfit reads it,
// which is what lets a level dress her from its own geometry - TR4's Angkor Wat
// carries the torso young Lara wears before she picks up her backpack. nullptr
// gives the outfit's mesh back. The head is the exception: a combat or speech
// face replaces it directly, and takes the head back from an override with it.
// Dropped at level end, along with the meshes it could name.
void Lara_Skin_SetMeshOverride(LARA_MESH mesh, OBJECT_MESH *mesh_ptr);
OBJECT_MESH *Lara_Skin_GetMeshOverride(LARA_MESH mesh);

void Lara_Skin_SetCombatFace(bool enabled);
// The speech face Lara wears, or -1 for her outfit's own. Remembered, so that
// changing outfit mid-sentence puts the new outfit's face on rather than
// leaving the old one.
void Lara_Skin_SetSpeechFace(int32_t index);
int32_t Lara_Skin_GetSpeechFace(void);
void Lara_Skin_SwapAllExtra(LARA_EXTRA_STATE state);
void Lara_Skin_SwapSingleExtra(LARA_MESH mesh, LARA_EXTRA_STATE state);
const ANIM_BONE *Lara_Skin_GetBoneBase(void);

bool Lara_Skin_IsBraidSupported(void);
const LARA_SKIN_BRAID *Lara_Skin_GetBraid(void);
int32_t Lara_Skin_GetBraidMeshIdx(int32_t braid_idx);
const ANIM_BONE *Lara_Skin_GetBraidBoneBase(int32_t braid_idx);

bool Lara_Skin_AreHolstersVisible(void);
void Lara_Skin_SetHolstersVisible(bool visible);
void Lara_Skin_ClearEquipment(LARA_MESH mesh);
void Lara_Skin_SetGunEquipment(LARA_MESH mesh, LARA_GUN_TYPE gun_type);
void Lara_Skin_SetExtraEquipment(
    LARA_MESH mesh, LARA_SKIN_EXTRA_MESH extra_mesh);
const LARA_SKIN_EQUIPMENT *Lara_Skin_GetEquipment(LARA_MESH mesh);

SAMPLE_SLOT Lara_Skin_GetAnimSFX(SAMPLE_SLOT sample_id);
