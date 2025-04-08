#pragma once

#include <stdint.h>

// Remembers the slot used when the player starts a loaded game.
// Persists across level reloads.
void Savegame_BindSlot(int32_t slot_num);

// Removes the binding of the current slot. Used when the player exits to
// title, issues a command like `/play` etc.
void Savegame_UnbindSlot(void);

// Returns the currently bound slot number. If there is none, returns -1.
int32_t Savegame_GetBoundSlot(void);

extern int32_t Savegame_GetSlotCount(void);
extern bool Savegame_IsSlotFree(int32_t slot_num);
extern bool Savegame_Load(int32_t slot_num);
extern bool Savegame_Save(int32_t slot_num);
