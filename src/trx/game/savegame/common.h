#pragma once

#include <trx/game/savegame/types.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer. Then the engine continues to execute the
// normal game flow and loads the specified level. Second phase occurs after
// everything finishes loading, e.g. items, creatures, triggers etc., and is
// what sets Lara's health, creatures status, triggers, inventory etc.

void Savegame_Init(void);

SAVEGAME_VERSION Savegame_GetInitialVersion(void);
void Savegame_SetInitialVersion(SAVEGAME_VERSION version);

// Whether the player can save the game at will. Save crystal modes reserve
// saving for the crystals themselves.
bool Savegame_IsManualSaveAllowed(void);
bool Savegame_RestartAvailable(SAVEGAME_SLOT_REF slot);

void Savegame_ProcessItemsBeforeLoad(void);
void Savegame_ProcessItemsBeforeSave(void);
bool Savegame_Load(SAVEGAME_SLOT_REF slot);
bool Savegame_Save(SAVEGAME_SLOT_REF slot);
bool Savegame_UpdateDeathCounters(SAVEGAME_SLOT_REF slot, int32_t death_count);
bool Savegame_LoadOnlyResumeInfo(SAVEGAME_SLOT_REF slot);
