#pragma once

#include <trx/filesystem.h>
#include <trx/game/savegame/file_read_io.h>
#include <trx/game/savegame/file_write_io.h>
#include <trx/game/savegame/types.h>
#include <trx/json.h>

#include <stdint.h>

const char *SG_File_GetSaveFilePattern(void);
bool SG_File_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
bool SG_File_LoadFromFile(MYFILE *fp);
bool SG_File_LoadOnlyResumeInfo(MYFILE *fp);
void SG_File_SaveToFile(MYFILE *fp, SAVEGAME_INFO *info);
bool SG_File_UpdateDeathCounters(
    MYFILE *fp, int32_t level_num, int32_t death_count);

// Start of reader functions ===================================================
bool SG_File_LoadLara(SG_READ_IO *io);
bool SG_File_LoadInventory(SG_READ_IO *io);
bool SG_File_LoadFlipmaps(SG_READ_IO *io);
bool SG_File_LoadCameras(SG_READ_IO *io);
bool SG_File_LoadItems(SG_READ_IO *io);
bool SG_File_LoadEffects(SG_READ_IO *io);
bool SG_File_LoadFlares(SG_READ_IO *io);
bool SG_File_LoadMusic(SG_READ_IO *io);
bool SG_File_LoadResumeInfoList(SG_READ_IO *io);
bool SG_File_LoadMisc(SG_READ_IO *io);
// End of reader functions =====================================================

// Start of writer functions ===================================================
void SG_File_DumpFlares(SG_WRITE_IO *io);
void SG_File_DumpEffects(SG_WRITE_IO *io);
void SG_File_DumpInventory(SG_WRITE_IO *io);
void SG_File_DumpFlipmaps(SG_WRITE_IO *io);
void SG_File_DumpCameras(SG_WRITE_IO *io);
void SG_File_DumpMusic(SG_WRITE_IO *io);
void SG_File_DumpItems(SG_WRITE_IO *io);
void SG_File_DumpLara(SG_WRITE_IO *io);
void SG_File_DumpResumeInfoList(SG_WRITE_IO *io);
void SG_File_DumpMisc(SG_WRITE_IO *io);
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
