#pragma once

#include <trx/game/savegame/types.h>

// The save slots: how many there are in each pool, which of them hold a game,
// and how to name one. A slot is addressed by a SAVEGAME_SLOT_REF rather than a
// bare index, so the two pools cannot be mistaken for one another.

void Savegame_Manager_Init(void);
void Savegame_Manager_Shutdown(void);
bool Savegame_IsInitialised(void);

// Rebuilds the slot tables after the configured slot counts change. The
// playthrough's resume info is not tied to the slot counts and stays as it is;
// rebuilding it would lose the stats and the loadout gathered so far.
void Savegame_ResizeSlots(void);

void Savegame_ScanSavedGames(void);
const SAVEGAME_INFO *Savegame_GetSavegameInfo(SAVEGAME_SLOT_REF slot);
bool Savegame_Delete(SAVEGAME_SLOT_REF slot);

// Writes the game the resume info now holds into the slot's file, and takes
// the slot into the table. The file keeps what it had if the write fails.
bool Savegame_Manager_WriteSlot(SAVEGAME_SLOT_REF slot);

int32_t Savegame_GetCounter(void);
int32_t Savegame_GetTotalCount(void);
int32_t Savegame_GetLevelNumber(SAVEGAME_SLOT_REF slot);
bool Savegame_IsSlotFree(SAVEGAME_SLOT_REF slot);

// Remembers the slot used when the player starts a loaded game.
// Persists across level reloads.
void Savegame_BindSlot(SAVEGAME_SLOT_REF slot);

// Removes the binding of the current slot. Used when the player exits to
// title, issues a command like `/play` etc.
void Savegame_UnbindSlot(void);

// Returns the currently bound slot. If there is none, returns an invalid slot.
SAVEGAME_SLOT_REF Savegame_GetBoundSlot(void);

// Returns the most recently created save slot number. If there is none,
// returns an invalid slot.
SAVEGAME_SLOT_REF Savegame_GetMostRecentlyCreatedSlot(void);

// Returns the most recently used slot save number. If there is none, returns
// an invalid slot.
SAVEGAME_SLOT_REF Savegame_GetMostRecentlyUsedSlot(void);

int32_t Savegame_GetSlotCount(SAVEGAME_SLOT_POOL pool);
SAVEGAME_SLOT_REF Savegame_GetNextQuickSlot(void);
int32_t Savegame_GetQuickVisualCount(void);
SAVEGAME_SLOT_REF Savegame_QuickFromVisualIndex(int32_t visual_index);
int32_t Savegame_QuickToVisualIndex(SAVEGAME_SLOT_REF slot);
bool Savegame_IsValidSlotRef(SAVEGAME_SLOT_REF slot);
SAVEGAME_SLOT_REF Savegame_NormalSlot(int32_t index);
SAVEGAME_SLOT_REF Savegame_QuickSlot(int32_t index);
SAVEGAME_SLOT_REF Savegame_InvalidSlot(void);
int32_t Savegame_SlotToParam(SAVEGAME_SLOT_REF slot);
SAVEGAME_SLOT_REF Savegame_SlotFromParam(int32_t param);
