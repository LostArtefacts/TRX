#pragma once

#include <trx/core/file.h>
#include <trx/core/filesystem.h>
#include <trx/core/json.h>
#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>

#include <stddef.h>
#include <stdint.h>

// Drops the level checksums held from earlier calls. The checksums are kept
// against the address of each level, so they must be dropped before the game
// flow frees its levels; otherwise a level allocated at a reused address
// reads back the checksum of the level that stood there before it.
void LevelCache_Reset(void);

uint64_t LevelCache_InitChecksum(const char *scope, uint32_t version);
uint64_t LevelCache_UpdateLevelChecksum(
    uint64_t checksum, const GF_LEVEL *level);

const char *LevelCache_GetLevelKey(const GF_LEVEL *level);

TRX_FILE *LevelCache_OpenBinaryRead(const char *filename, uint64_t checksum);
TRX_FILE *LevelCache_OpenBinaryWrite(const char *filename, uint64_t checksum);

// Reads what was cached for the given level data. A cache that is not there
// or was written for other data reads as nothing and is no fault; one that
// cannot be read is reported.
RESULT LevelCache_ReadJSON(
    const char *filename, uint64_t checksum, JSON_VALUE **out_root);
RESULT LevelCache_WriteJSON(
    const char *filename, uint64_t checksum, JSON_VALUE *root);
