#pragma once

#include "../../json.h"

#include <stdint.h>

// Start of helper functions ===================================================
// TODO: make these private eventually.
typedef struct SAVEGAME_BSON_READ_CONTEXT SAVEGAME_BSON_READ_CONTEXT;
SAVEGAME_BSON_READ_CONTEXT *Savegame_BSON_StartRead(JSON_VALUE *root);
void Savegame_BSON_FinishRead(SAVEGAME_BSON_READ_CONTEXT *ctx, bool success);
bool Savegame_BSON_LoadInventory(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadFlipmaps(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadCameras(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadEffects(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadFlares(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadMusic(
    SAVEGAME_BSON_READ_CONTEXT *ctx, uint16_t header_version);
// End of helper functions =====================================================

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    int16_t initial_version;
    uint16_t version;
    int32_t compressed_size;
    int32_t uncompressed_size;
} SAVEGAME_BSON_HEADER;

typedef struct {
    uint32_t flags;
    int32_t counter;
    int32_t level_num;
    int32_t title_size;
} SAVEGAME_BSON_EXTENDED_HEADER;
#pragma pack(pop)
