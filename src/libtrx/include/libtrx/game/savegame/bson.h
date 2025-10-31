#pragma once

#include "../../filesystem.h"
#include "../../json.h"
#include "./types.h"

#include <stdint.h>

// Start of helper functions ===================================================
// TODO: make these private eventually.
JSON_VALUE *Savegame_BSON_ReadRaw(MYFILE *fp, int32_t *version_out);
bool Savegame_BSON_LoadFromFile(MYFILE *fp);
bool Savegame_BSON_LoadOnlyResumeInfo(MYFILE *fp);
bool Savegame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);

typedef struct SAVEGAME_BSON_READ_CONTEXT SAVEGAME_BSON_READ_CONTEXT;
SAVEGAME_BSON_READ_CONTEXT *Savegame_BSON_StartRead(JSON_VALUE *root);
void Savegame_BSON_FinishRead(SAVEGAME_BSON_READ_CONTEXT *ctx, bool success);
bool Savegame_BSON_LoadLara(
    SAVEGAME_BSON_READ_CONTEXT *ctx, uint16_t header_version);
bool Savegame_BSON_LoadInventory(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadFlipmaps(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadCameras(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadItems(
    SAVEGAME_BSON_READ_CONTEXT *ctx, uint16_t header_version);
bool Savegame_BSON_LoadEffects(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadFlares(SAVEGAME_BSON_READ_CONTEXT *ctx);
bool Savegame_BSON_LoadMusic(
    SAVEGAME_BSON_READ_CONTEXT *ctx, uint16_t header_version);
bool Savegame_BSON_LoadResumeInfoList(
    SAVEGAME_BSON_READ_CONTEXT *ctx, uint16_t header_version);
bool Savegame_BSON_LoadMisc(SAVEGAME_BSON_READ_CONTEXT *ctx);

typedef struct SAVEGAME_BSON_WRITE_CONTEXT SAVEGAME_BSON_WRITE_CONTEXT;
SAVEGAME_BSON_WRITE_CONTEXT *Savegame_BSON_StartWrite(void);
void Savegame_BSON_FinishWrite(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
JSON_OBJECT *Savegame_BSON_GetWriteRoot(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
void Savegame_BSON_DumpFlares(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
void Savegame_BSON_DumpEffects(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
void Savegame_BSON_DumpInventory(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
void Savegame_BSON_DumpFlipmaps(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
void Savegame_BSON_DumpCameras(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
void Savegame_BSON_DumpMusic(SAVEGAME_BSON_WRITE_CONTEXT *ctx);
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
