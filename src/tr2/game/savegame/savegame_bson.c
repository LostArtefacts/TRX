#include "game/savegame.h"

#include <libtrx/bson.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/effects.h>
#include <libtrx/game/game.h>
#include <libtrx/game/game_flow.h>
#include <libtrx/game/inventory.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/general/lift.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/objects/traps/sliding_pillar.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/objects/vehicles/boat.h>
#include <libtrx/game/objects/vehicles/skidoo_common.h>
#include <libtrx/game/output.h>
#include <libtrx/game/pathing.h>
#include <libtrx/game/savegame/bson.h>
#include <libtrx/game/shell.h>
#include <libtrx/game/stats.h>
#include <libtrx/json.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>
#include <libtrx/version.h>

#include <string.h>
#include <zlib.h>

#define SAVEGAME_BSON_MAGIC MKTAG('T', '2', 'X', 'B')

static const char *M_GetSaveFilePattern(void);
static void M_SaveToFile(MYFILE *fp, SAVEGAME_INFO *info);
static bool M_UpdateDeathCounters(
    MYFILE *fp, int32_t level_num, int32_t death_count);

static SAVEGAME_STRATEGY m_Strategy = {
    // clang-format off
    .allow_load = true,
    .allow_save = true,
    .format = SAVEGAME_FORMAT_BSON,
    .get_save_file_pattern_func = M_GetSaveFilePattern,
    .fill_info_func = Savegame_BSON_FillInfo,
    .load_from_file_func = Savegame_BSON_LoadFromFile,
    .save_to_file_func = M_SaveToFile,
    .load_only_resume_info_func = Savegame_BSON_LoadOnlyResumeInfo,
    .update_death_counters_func = M_UpdateDeathCounters,
    // clang-format on
};

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_bson;
}

static void M_SaveRaw(
    MYFILE *const fp, const JSON_VALUE *const root, const int32_t level_num)
{
    size_t uncompressed_size;
    char *uncompressed = BSON_Write(root, &uncompressed_size);

    uLongf compressed_size = compressBound(uncompressed_size);
    char *compressed = Memory_Alloc(compressed_size);
    const int32_t result = compress(
        (Bytef *)compressed, &compressed_size, (const Bytef *)uncompressed,
        (uLongf)uncompressed_size);
    if (result != Z_OK) {
        Shell_ExitSystem("Failed to compress savegame data");
    }

    const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
    const SAVEGAME_BSON_HEADER header = {
        .magic = SAVEGAME_BSON_MAGIC,
        .initial_version = Savegame_GetInitialVersion(),
        .version = SAVEGAME_CURRENT_VERSION,
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };
    const SAVEGAME_BSON_EXTENDED_HEADER extra_header = {
        .flags = Game_GetBonusFlag(),
        .counter = Savegame_GetCounter(),
        .level_num = level->num,
        .title_size = strlen(level->title),
    };

    File_WriteData(fp, &header, sizeof(header));
    File_WriteData(fp, compressed, compressed_size);
    File_WriteData(fp, &extra_header, sizeof(extra_header));
    File_WriteData(fp, level->title, strlen(level->title));

    Memory_FreePointer(&uncompressed);
    Memory_FreePointer(&compressed);
}

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    SAVEGAME_BSON_WRITE_CONTEXT *const ctx = Savegame_BSON_StartWrite();
    JSON_VALUE *const root = Savegame_BSON_GetWriteRoot(ctx);

    Savegame_BSON_DumpResumeInfoList(ctx);
    Savegame_BSON_DumpInventory(ctx);
    Savegame_BSON_DumpFlipmaps(ctx);
    Savegame_BSON_DumpCameras(ctx);
    Savegame_BSON_DumpItems(ctx);
    Savegame_BSON_DumpEffects(ctx);
    Savegame_BSON_DumpLara(ctx);
    Savegame_BSON_DumpMusic(ctx);
    Savegame_BSON_DumpFlares(ctx);
    Savegame_BSON_DumpMisc(ctx);

    M_SaveRaw(fp, root, current_level->num);
    Savegame_BSON_FinishWrite(ctx);
}

static bool M_UpdateDeathCounters(
    MYFILE *const fp, int32_t level_num, const int32_t death_count)
{
    bool result = false;
    int32_t version;
    JSON_VALUE *const root = Savegame_BSON_ReadRaw(fp, &version);
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        LOG_ERROR("Cannot find the root object");
        goto cleanup;
    }

    JSON_OBJECT *const misc_obj = JSON_ObjectGetObject(root_obj, "misc");
    if (misc_obj == nullptr) {
        LOG_ERROR("Cannot find the misc object");
        goto cleanup;
    }
    JSON_ObjectEvictKey(misc_obj, "death_count");
    JSON_ObjectAppendInt(misc_obj, "death_count", death_count);

    File_Seek(fp, 0, FILE_SEEK_SET);
    M_SaveRaw(fp, root, level_num);
    result = true;

cleanup:
    JSON_ValueFree(root);
    return result;
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
