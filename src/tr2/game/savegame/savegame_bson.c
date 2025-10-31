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

static JSON_OBJECT *M_DumpMisc(void)
{
    JSON_OBJECT *const misc_obj = JSON_ObjectNew();
    JSON_ObjectAppendString(misc_obj, "game_version", g_TRXVersion);
    JSON_ObjectAppendInt(misc_obj, "bonus_flag", Game_GetBonusFlag());
    JSON_ObjectAppendBool(
        misc_obj, "are_monks_angry", Creature_AreAlliesHostile());
    JSON_ObjectAppendInt(misc_obj, "sunset_timer", Output_GetSunsetTimer());
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    JSON_ObjectAppendInt(misc_obj, "death_count", resume->stats.death_count);
    return misc_obj;
}

static JSON_ARRAY *M_DumpResumeInfo(void)
{
    JSON_ARRAY *const resume_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        JSON_OBJECT *const resume_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(
            resume_obj, "lara_hitpoints", resume->lara_hitpoints);
        JSON_ObjectAppendInt(resume_obj, "pistol_ammo", resume->pistol_ammo);
        JSON_ObjectAppendInt(resume_obj, "magnum_ammo", resume->magnum_ammo);
        JSON_ObjectAppendInt(resume_obj, "uzi_ammo", resume->uzi_ammo);
        JSON_ObjectAppendInt(resume_obj, "shotgun_ammo", resume->shotgun_ammo);
        JSON_ObjectAppendInt(resume_obj, "m16_ammo", resume->m16_ammo);
        JSON_ObjectAppendInt(resume_obj, "grenade_ammo", resume->grenade_ammo);
        JSON_ObjectAppendInt(resume_obj, "harpoon_ammo", resume->harpoon_ammo);
        JSON_ObjectAppendInt(resume_obj, "num_medis", resume->small_medipacks);
        JSON_ObjectAppendInt(
            resume_obj, "num_big_medis", resume->large_medipacks);
        JSON_ObjectAppendInt(resume_obj, "num_flares", resume->flares);
        JSON_ObjectAppendInt(resume_obj, "num_scions", resume->num_scions);
        JSON_ObjectAppendInt(resume_obj, "gun_status", resume->gun_status);
        JSON_ObjectAppendInt(resume_obj, "gun_type", resume->equipped_gun_type);
        JSON_ObjectAppendInt(
            resume_obj, "holsters_gun_type", resume->holsters_gun_type);
        JSON_ObjectAppendInt(
            resume_obj, "back_gun_type", resume->back_gun_type);

        JSON_ObjectAppendBool(resume_obj, "available", resume->flags.available);
        JSON_ObjectAppendBool(
            resume_obj, "has_pistols", resume->flags.has_pistols);
        JSON_ObjectAppendBool(
            resume_obj, "has_magnums", resume->flags.has_magnums);
        JSON_ObjectAppendBool(resume_obj, "has_uzis", resume->flags.has_uzis);
        JSON_ObjectAppendBool(
            resume_obj, "has_shotgun", resume->flags.has_shotgun);
        JSON_ObjectAppendBool(resume_obj, "has_m16", resume->flags.has_m16);
        JSON_ObjectAppendBool(
            resume_obj, "has_grenade", resume->flags.has_grenade);
        JSON_ObjectAppendBool(
            resume_obj, "has_harpoon", resume->flags.has_harpoon);

        JSON_ObjectAppendInt(resume_obj, "timer", resume->stats.timer);
        JSON_ObjectAppendInt(resume_obj, "ammo_hits", resume->stats.ammo_hits);
        JSON_ObjectAppendInt(resume_obj, "ammo_used", resume->stats.ammo_used);
        JSON_ObjectAppendInt(
            resume_obj, "distance_travelled", resume->stats.distance_travelled);
        JSON_ObjectAppendInt(resume_obj, "kills", resume->stats.kill_count);
        JSON_ObjectAppendInt(resume_obj, "secrets", resume->stats.secret_flags);
        JSON_ObjectAppendInt(resume_obj, "pickups", resume->stats.pickup_count);
        JSON_ObjectAppendDouble(
            resume_obj, "medipacks_used", resume->stats.medipacks_used);
        JSON_ArrayAppendObject(resume_arr, resume_obj);
    }
    return resume_arr;
}

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    SAVEGAME_BSON_WRITE_CONTEXT *const ctx = Savegame_BSON_StartWrite();
    JSON_OBJECT *const root_obj = Savegame_BSON_GetWriteRoot(ctx);

    JSON_ObjectAppendObject(root_obj, "misc", M_DumpMisc());
    JSON_ObjectAppendArray(root_obj, "resume_info", M_DumpResumeInfo());
    Savegame_BSON_DumpInventory(ctx);
    Savegame_BSON_DumpFlipmaps(ctx);
    Savegame_BSON_DumpCameras(ctx);
    Savegame_BSON_DumpItems(ctx);
    Savegame_BSON_DumpEffects(ctx);
    Savegame_BSON_DumpLara(ctx);
    Savegame_BSON_DumpMusic(ctx);
    Savegame_BSON_DumpFlares(ctx);

    JSON_VALUE *const root = JSON_ValueFromObject(root_obj);
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
