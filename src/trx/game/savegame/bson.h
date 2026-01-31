#pragma once

#include <trx/filesystem.h>
#include <trx/game/savegame/bson_read_io.h>
#include <trx/game/savegame/bson_write_io.h>
#include <trx/game/savegame/types.h>
#include <trx/json.h>

#include <stdint.h>

// Start of reader functions ===================================================
bool Savegame_BSON_LoadLara(SG_READ_IO *io);
bool Savegame_BSON_LoadInventory(SG_READ_IO *io);
bool Savegame_BSON_LoadFlipmaps(SG_READ_IO *io);
bool Savegame_BSON_LoadCameras(SG_READ_IO *io);
bool Savegame_BSON_LoadItems(SG_READ_IO *io);
bool Savegame_BSON_LoadEffects(SG_READ_IO *io);
bool Savegame_BSON_LoadFlares(SG_READ_IO *io);
bool Savegame_BSON_LoadMusic(SG_READ_IO *io);
bool Savegame_BSON_LoadResumeInfoList(SG_READ_IO *io);
bool Savegame_BSON_LoadMisc(SG_READ_IO *io);
// End of reader functions =====================================================

// Start of writer functions ===================================================
void Savegame_BSON_DumpFlares(SG_WRITE_IO *io);
void Savegame_BSON_DumpEffects(SG_WRITE_IO *io);
void Savegame_BSON_DumpInventory(SG_WRITE_IO *io);
void Savegame_BSON_DumpFlipmaps(SG_WRITE_IO *io);
void Savegame_BSON_DumpCameras(SG_WRITE_IO *io);
void Savegame_BSON_DumpMusic(SG_WRITE_IO *io);
void Savegame_BSON_DumpItems(SG_WRITE_IO *io);
void Savegame_BSON_DumpLara(SG_WRITE_IO *io);
void Savegame_BSON_DumpResumeInfoList(SG_WRITE_IO *io);
void Savegame_BSON_DumpMisc(SG_WRITE_IO *io);
// End of writer functions =====================================================

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
