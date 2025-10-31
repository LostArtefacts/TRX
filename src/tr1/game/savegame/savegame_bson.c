#include "game/savegame.h"

#include <libtrx/bson.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/effects.h>
#include <libtrx/game/game.h>
#include <libtrx/game/game_flow.h>
#include <libtrx/game/gun/rifle.h>
#include <libtrx/game/inventory.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/general/lift.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/objects/traps/sliding_pillar.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/objects/vehicles/boat.h>
#include <libtrx/game/objects/vehicles/skidoo_common.h>
#include <libtrx/game/pathing.h>
#include <libtrx/game/savegame/bson.h>
#include <libtrx/game/shell.h>
#include <libtrx/game/stats.h>
#include <libtrx/json.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>
#include <libtrx/version.h>

#include <inttypes.h>
#include <stdio.h>
#include <zconf.h>
#include <zlib.h>

#define SAVEGAME_BSON_MAGIC MKTAG('T', '1', 'M', 'B')

static const char *M_GetSaveFilePattern(void);
static void M_SaveToFile(MYFILE *fp, SAVEGAME_INFO *savegame_info);
static bool M_UpdateDeathCounters(
    MYFILE *fp, int32_t level_num, int32_t death_count);

static SAVEGAME_STRATEGY m_Strategy = {
    .allow_load = true,
    .allow_save = true,
    .format = SAVEGAME_FORMAT_BSON,
    .get_save_file_pattern_func = M_GetSaveFilePattern,
    .fill_info_func = Savegame_BSON_FillInfo,
    .load_from_file_func = Savegame_BSON_LoadFromFile,
    .load_only_resume_info_func = Savegame_BSON_LoadOnlyResumeInfo,
    .save_to_file_func = M_SaveToFile,
    .update_death_counters_func = M_UpdateDeathCounters,
};

static void M_SaveRaw(
    MYFILE *fp, JSON_VALUE *root, int32_t version, const int32_t level_num)
{
    size_t uncompressed_size;
    char *uncompressed = BSON_Write(root, &uncompressed_size);

    uLongf compressed_size = compressBound(uncompressed_size);
    char *compressed = Memory_Alloc(compressed_size);
    if (compress(
            (Bytef *)compressed, &compressed_size, (const Bytef *)uncompressed,
            (uLongf)uncompressed_size)
        != Z_OK) {
        Shell_ExitSystem("Failed to compress savegame data");
    }

    Memory_FreePointer(&uncompressed);

    const SAVEGAME_BSON_HEADER header = {
        .magic = SAVEGAME_BSON_MAGIC,
        .initial_version = Savegame_GetInitialVersion(),
        .version = version,
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };
    File_WriteData(fp, &header, sizeof(header));

    File_WriteData(fp, compressed, compressed_size);

    const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
    const SAVEGAME_BSON_EXTENDED_HEADER extra_header = {
        .flags = Game_GetBonusFlag(),
        .counter = Savegame_GetCounter(),
        .level_num = level->num,
        .title_size = strlen(level->title),
    };
    File_WriteData(fp, &extra_header, sizeof(extra_header));
    File_WriteData(fp, level->title, strlen(level->title));

    Memory_FreePointer(&compressed);
}

static JSON_OBJECT *M_DumpMisc(void)
{
    JSON_OBJECT *misc_obj = JSON_ObjectNew();
    JSON_ObjectAppendString(misc_obj, "game_version", g_TRXVersion);
    JSON_ObjectAppendInt(misc_obj, "bonus_flag", Game_GetBonusFlag());

    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    JSON_ObjectAppendInt(misc_obj, "death_count", resume->stats.death_count);
    return misc_obj;
}

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_bson;
}

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const savegame_info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    SAVEGAME_BSON_WRITE_CONTEXT *const ctx = Savegame_BSON_StartWrite();
    JSON_OBJECT *const root_obj = Savegame_BSON_GetWriteRoot(ctx);

    JSON_ObjectAppendString(root_obj, "level_title", current_level->title);
    JSON_ObjectAppendInt(root_obj, "save_counter", Savegame_GetCounter());
    JSON_ObjectAppendInt(root_obj, "level_num", current_level->num);

    JSON_ObjectAppendObject(root_obj, "misc", M_DumpMisc());
    Savegame_BSON_DumpResumeInfoList(ctx);
    Savegame_BSON_DumpInventory(ctx);
    Savegame_BSON_DumpFlipmaps(ctx);
    Savegame_BSON_DumpCameras(ctx);
    Savegame_BSON_DumpItems(ctx);
    Savegame_BSON_DumpEffects(ctx);
    Savegame_BSON_DumpLara(ctx);
    Savegame_BSON_DumpMusic(ctx);
    Savegame_BSON_DumpFlares(ctx);

    JSON_VALUE *const root = JSON_ValueFromObject(root_obj);
    M_SaveRaw(fp, root, SAVEGAME_CURRENT_VERSION, current_level->num);
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
    M_SaveRaw(fp, root, version, level_num);
    result = true;

cleanup:
    JSON_ValueFree(root);
    return result;
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
