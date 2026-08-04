#pragma once

#include <trx/game/savegame/types.h>

// The save slots: how many there are in each pool, which of them hold a game,
// and how to name one. A slot is addressed by a SAVEGAME_SLOT_REF rather than a
// bare index, so the two pools cannot be mistaken for one another.

void SG_Manager_Init(void);
void SG_Manager_Shutdown(void);
bool SG_Manager_IsInitialised(void);

// Rebuilds the slot tables after the configured slot counts change. The
// playthrough's resume info is not tied to the slot counts and stays as it is;
// rebuilding it would lose the stats and the loadout gathered so far.
void SG_Manager_ResizeSlots(void);

void SG_Manager_ScanSavedGames(void);
const SAVEGAME_INFO *SG_Manager_GetSavegameInfo(SAVEGAME_SLOT_REF slot);
bool SG_Manager_Delete(SAVEGAME_SLOT_REF slot);

// Writes the game the resume info now holds into the slot's file, and takes
// the slot into the table. The file keeps what it had if the write fails.
bool SG_Manager_WriteSlot(SAVEGAME_SLOT_REF slot);

int32_t SG_Manager_GetCounter(void);
int32_t SG_Manager_GetTotalCount(void);
int32_t SG_Manager_GetLevelNumber(SAVEGAME_SLOT_REF slot);
bool SG_Manager_IsSlotFree(SAVEGAME_SLOT_REF slot);

// Remembers the slot used when the player starts a loaded game.
// Persists across level reloads.
void SG_Manager_BindSlot(SAVEGAME_SLOT_REF slot);

// Removes the binding of the current slot. Used when the player exits to
// title, issues a command like `/play` etc.
void SG_Manager_UnbindSlot(void);

// Returns the currently bound slot. If there is none, returns an invalid slot.
SAVEGAME_SLOT_REF SG_Manager_GetBoundSlot(void);

// Returns the most recently created save slot number. If there is none,
// returns an invalid slot.
SAVEGAME_SLOT_REF SG_Manager_GetMostRecentlyCreatedSlot(void);

// Returns the most recently used slot save number. If there is none, returns
// an invalid slot.
SAVEGAME_SLOT_REF SG_Manager_GetMostRecentlyUsedSlot(void);

int32_t SG_Manager_GetSlotCount(SAVEGAME_SLOT_POOL pool);
SAVEGAME_SLOT_REF SG_Manager_GetNextQuickSlot(void);
int32_t SG_Manager_GetQuickVisualCount(void);
SAVEGAME_SLOT_REF SG_Manager_QuickFromVisualIndex(int32_t visual_index);
int32_t SG_Manager_QuickToVisualIndex(SAVEGAME_SLOT_REF slot);
bool SG_Manager_IsValidSlotRef(SAVEGAME_SLOT_REF slot);
SAVEGAME_SLOT_REF SG_Manager_NormalSlot(int32_t index);
SAVEGAME_SLOT_REF SG_Manager_QuickSlot(int32_t index);
SAVEGAME_SLOT_REF SG_Manager_InvalidSlot(void);
int32_t SG_Manager_SlotToParam(SAVEGAME_SLOT_REF slot);
SAVEGAME_SLOT_REF SG_Manager_SlotFromParam(int32_t param);
