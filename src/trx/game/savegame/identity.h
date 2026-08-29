#pragma once

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/result.h>
#include <trx/game/catalog/manager.h>

// Write an identity as its slot and key. Return false if neither form exists.
bool SaveGame_WriteIdentity(
    JSON_WRITE_IO *io, const char *slot_field, const char *key_field,
    CATALOG_CONTEXT context, CATALOG_ID id);

// Read an identity by key when present, otherwise by slot. Report failure when
// neither form resolves.
RESULT SaveGame_ReadIdentity(
    JSON_READ_IO *io, const char *slot_field, const char *key_field,
    CATALOG_CONTEXT context, CATALOG_ID *out_id);

// Record one savegame record that this game cannot place.
void SaveGame_NoteDropped(const char *what);

// Return the number of unplaced records for the current load.
int32_t SaveGame_GetDroppedCount(void);

// Clear the unplaced-record count.
void SaveGame_ResetDropped(void);
