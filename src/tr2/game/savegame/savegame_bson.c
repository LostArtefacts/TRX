#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "global/types_decomp.h"
#include "global/vars.h"

#include <libtrx/bson.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/general/lift.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/objects/traps/sliding_pillar.h>
#include <libtrx/game/objects/vars.h>
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

typedef struct {
    int16_t count;
    int16_t id_map[MAX_EFFECTS];
} SAVEGAME_BSON_FX_ORDER;

#define DUMP_XYZ(obj, key, value)                                              \
    do {                                                                       \
        JSON_OBJECT *const sub_obj = JSON_ObjectNew();                         \
        JSON_ObjectAppendInt(sub_obj, "x", value.x);                           \
        JSON_ObjectAppendInt(sub_obj, "y", value.y);                           \
        JSON_ObjectAppendInt(sub_obj, "z", value.z);                           \
        JSON_ObjectAppendObject(obj, key, sub_obj);                            \
    } while (0)

#define LOAD_XYZ(obj, key, value)                                              \
    do {                                                                       \
        const JSON_OBJECT *const sub_obj = JSON_ObjectGetObject(obj, key);     \
        value.x = JSON_ObjectGetInt(sub_obj, "x", value.x);                    \
        value.y = JSON_ObjectGetInt(sub_obj, "y", value.y);                    \
        value.z = JSON_ObjectGetInt(sub_obj, "z", value.z);                    \
    } while (0)

static const char *M_GetSaveFilePattern(void);
static bool M_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
static void M_SaveToFile(MYFILE *fp, SAVEGAME_INFO *info);
static bool M_LoadFromFile(MYFILE *fp);

static SAVEGAME_STRATEGY m_Strategy = {
    // clang-format off
    .allow_load = true,
    .allow_save = true,
    .format = SAVEGAME_FORMAT_BSON,
    .get_save_file_pattern_func = M_GetSaveFilePattern,
    .fill_info_func = M_FillInfo,
    .load_from_file_func = M_LoadFromFile,
    .save_to_file_func = M_SaveToFile,
    .load_only_resume_info_func = nullptr,
    .update_death_counters_func = nullptr,
    // clang-format on
};

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_bson;
}

static bool M_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, &header, sizeof(SAVEGAME_BSON_HEADER));
    info->initial_version = header.initial_version;
    info->features.restart = false;
    info->features.select_level = false;

    File_Skip(fp, header.compressed_size);
    SAVEGAME_BSON_EXTENDED_HEADER extra_header;
    File_ReadData(fp, &extra_header, sizeof(extra_header));

    info->counter = extra_header.counter;
    info->level_num = extra_header.level_num;
    if (extra_header.title_size >= (int32_t)File_Size(fp)) {
        return false;
    }
    info->level_title = Memory_Alloc(extra_header.title_size + 1);
    File_ReadData(fp, info->level_title, extra_header.title_size);
    return true;
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

static JSON_VALUE *M_ParseFromBuffer(
    const char *const buffer, int32_t *const version_out)
{
    const SAVEGAME_BSON_HEADER *const header = (SAVEGAME_BSON_HEADER *)buffer;
    if (header->magic != SAVEGAME_BSON_MAGIC) {
        LOG_ERROR("Invalid savegame magic");
        return nullptr;
    }

    if (version_out != nullptr) {
        *version_out = header->version;
    }

    const char *compressed = buffer + sizeof(SAVEGAME_BSON_HEADER);
    char *uncompressed = Memory_Alloc(header->uncompressed_size);

    uLongf uncompressed_size = header->uncompressed_size;
    const int32_t error_code = uncompress(
        (Bytef *)uncompressed, &uncompressed_size, (const Bytef *)compressed,
        (uLongf)header->compressed_size);
    if (error_code != Z_OK) {
        LOG_ERROR("Failed to decompress the data (error %d)", error_code);
        Memory_FreePointer(&uncompressed);
        return nullptr;
    }

    JSON_VALUE *const root = BSON_Parse(uncompressed, uncompressed_size);
    Memory_FreePointer(&uncompressed);
    return root;
}

static JSON_VALUE *M_ReadRaw(MYFILE *const fp, int32_t *const version_out)
{
    const size_t buffer_size = File_Size(fp);
    char *buffer = Memory_Alloc(buffer_size);
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, buffer_size);

    JSON_VALUE *const result = M_ParseFromBuffer(buffer, version_out);
    Memory_FreePointer(&buffer);
    return result;
}

static JSON_OBJECT *M_DumpMisc(void)
{
    JSON_OBJECT *const misc_obj = JSON_ObjectNew();
    JSON_ObjectAppendString(misc_obj, "game_version", g_TRXVersion);
    JSON_ObjectAppendInt(misc_obj, "bonus_flag", Game_GetBonusFlag());
    JSON_ObjectAppendBool(
        misc_obj, "are_monks_angry", Creature_AreAlliesHostile());
    JSON_ObjectAppendInt(misc_obj, "sunset_timer", Output_GetSunsetTimer());
    return misc_obj;
}

static bool M_LoadMisc(JSON_OBJECT *const misc_obj)
{
    if (misc_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing misc info");
        return false;
    }

    const int32_t bonus_flag = JSON_ObjectGetInt(misc_obj, "bonus_flag", 0);
    Game_SetBonusFlag(bonus_flag);
    const bool hostile = JSON_ObjectGetBool(misc_obj, "are_monks_angry", false);
    Creature_SetAlliesHostile(hostile);
    const int32_t sunset_timer = JSON_ObjectGetInt(misc_obj, "sunset_timer", 0);
    Output_SetSunsetTimer(sunset_timer);
    return true;
}

static JSON_OBJECT *M_DumpMusic(void)
{
    JSON_OBJECT *const music_obj = JSON_ObjectNew();
    JSON_ARRAY *const track_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        JSON_ArrayAppendInt(track_arr, Music_GetTrackFlags(i));
    }
    JSON_ObjectAppendArray(music_obj, "flags", track_arr);

    const MUSIC_ID current_track = Music_GetCurrentPlayingTrack();
    const MUSIC_ID current_ambient = Music_GetCurrentLoopedTrack();
    JSON_OBJECT *const current_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(current_obj, "current_track", current_track);
    JSON_ObjectAppendInt(current_obj, "current_ambient", current_ambient);
    JSON_ObjectAppendDouble(current_obj, "timestamp", Music_GetTimestamp());
    JSON_ObjectAppendObject(music_obj, "current", current_obj);

    return music_obj;
}

static MUSIC_ID M_ConvertMusicTrack(
    const MUSIC_ID track_id, const uint16_t header_version)
{
    if (track_id == MX_INACTIVE || header_version >= VERSION_11) {
        return track_id;
    }
    return Music_ConvertLegacyTrack(track_id);
}

static bool M_LoadMusic(
    JSON_OBJECT *const music_obj, const uint16_t header_version)
{
    if (music_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing music info");
        return false;
    }

    const JSON_ARRAY *const track_arr = JSON_ObjectGetArray(music_obj, "flags");
    if (track_arr == nullptr) {
        LOG_WARNING("Malformed save: invalid or missing music track array");
        return true;
    }

    if ((signed)track_arr->length > MAX_MUSIC_TRACKS) {
        LOG_WARNING(
            "Malformed save: expected at most %d music track flags, got %d",
            MAX_MUSIC_TRACKS, track_arr->length);
        return true;
    }

    for (int32_t i = 0; i < (signed)track_arr->length; i++) {
        const MUSIC_ID track_id = M_ConvertMusicTrack(i, header_version);
        Music_SetTrackFlags(track_id, JSON_ArrayGetInt(track_arr, i, 0));
    }

    const JSON_OBJECT *const current_obj =
        JSON_ObjectGetObject(music_obj, "current");
    if (current_obj == nullptr) {
        return true;
    }

    MUSIC_ID current_track =
        JSON_ObjectGetInt(current_obj, "current_track", MX_INACTIVE);
    MUSIC_ID ambient_track =
        JSON_ObjectGetInt(current_obj, "current_ambient", MX_INACTIVE);
    const double timestamp =
        JSON_ObjectGetDouble(current_obj, "timestamp", -1.0);

    if (header_version < VERSION_9) {
        const bool legacy_ambient =
            JSON_ObjectGetBool(current_obj, "is_ambient", false);
        if (legacy_ambient && current_track != MX_INACTIVE) {
            ambient_track = current_track;
        }
    }

    // Added in 1.2 after removing OG music track shifting, so allowing previous
    // saves to still load. Remove after a suitable period.
    current_track = M_ConvertMusicTrack(current_track, header_version);
    ambient_track = M_ConvertMusicTrack(ambient_track, header_version);

    Music_Stop();
    if (ambient_track != MX_INACTIVE) {
        // Always restart the ambient as it may have changed based on the
        // current position in the level.
        Music_Play_Direct(ambient_track, MPM_LOOPED);
    }

    if (g_Config.audio.music_load_condition == MUSIC_LOAD_NEVER) {
        return true;
    }

    const bool is_ambient =
        current_track != MX_INACTIVE && current_track == ambient_track;
    if (!is_ambient && current_track != MX_INACTIVE) {
        Music_Play_Direct(current_track, MPM_ALWAYS);
    }

    const bool load_timestamp =
        !is_ambient || g_Config.audio.music_load_condition == MUSIC_LOAD_ALWAYS;
    if (load_timestamp && !Music_SeekTimestamp(timestamp)) {
        LOG_WARNING(
            "Could not load current track %d at timestamp %" PRId64 ".",
            current_track, timestamp);
    }

    return true;
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
        JSON_ObjectAppendDouble(
            resume_obj, "medipacks_used", resume->stats.medipacks_used);
        JSON_ObjectAppendInt(
            resume_obj, "max_secrets", resume->stats.max_secret_count);
        JSON_ObjectAppendInt(
            resume_obj, "all_secrets_mask", resume->stats.all_secrets_mask);
        JSON_ArrayAppendObject(resume_arr, resume_obj);
    }
    return resume_arr;
}

static bool M_LoadResumeInfo(JSON_ARRAY *const resume_arr)
{
    if (resume_arr == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing resume array");
        return false;
    }
    const int32_t expected_length = GF_GetLevelTable(GFLT_MAIN)->count;
    if ((signed)resume_arr->length != expected_length) {
        LOG_ERROR(
            "Malformed save: expected %d resume info elements, got %d",
            expected_length, resume_arr->length);
        return false;
    }

    for (int32_t i = 0; i < expected_length; i++) {
        JSON_OBJECT *const resume_obj = JSON_ArrayGetObject(resume_arr, i);
        if (resume_obj == nullptr) {
            LOG_ERROR("Malformed save: invalid resume info");
            return false;
        }

        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        resume->lara_hitpoints = JSON_ObjectGetInt(
            resume_obj, "lara_hitpoints",
            g_Config.gameplay.start_lara_hitpoints);
        resume->pistol_ammo = JSON_ObjectGetInt(resume_obj, "pistol_ammo", 0);
        resume->magnum_ammo = JSON_ObjectGetInt(resume_obj, "magnum_ammo", 0);
        resume->uzi_ammo = JSON_ObjectGetInt(resume_obj, "uzi_ammo", 0);
        resume->shotgun_ammo = JSON_ObjectGetInt(resume_obj, "shotgun_ammo", 0);
        resume->m16_ammo = JSON_ObjectGetInt(resume_obj, "m16_ammo", 0);
        resume->grenade_ammo = JSON_ObjectGetInt(resume_obj, "grenade_ammo", 0);
        resume->harpoon_ammo = JSON_ObjectGetInt(resume_obj, "harpoon_ammo", 0);
        resume->small_medipacks = JSON_ObjectGetInt(resume_obj, "num_medis", 0);
        resume->large_medipacks =
            JSON_ObjectGetInt(resume_obj, "num_big_medis", 0);
        resume->flares = JSON_ObjectGetInt(resume_obj, "num_flares", 0);
        resume->gun_status =
            JSON_ObjectGetInt(resume_obj, "gun_status", LGS_ARMLESS);
        resume->equipped_gun_type =
            JSON_ObjectGetInt(resume_obj, "gun_type", LGT_UNARMED);
        resume->holsters_gun_type =
            JSON_ObjectGetInt(resume_obj, "holsters_gun_type", LGT_UNKNOWN);
        resume->back_gun_type =
            JSON_ObjectGetInt(resume_obj, "back_gun_type", LGT_UNKNOWN);

        resume->flags.available =
            JSON_ObjectGetBool(resume_obj, "available", 0);
        resume->flags.has_pistols =
            JSON_ObjectGetBool(resume_obj, "has_pistols", 0);
        resume->flags.has_magnums =
            JSON_ObjectGetBool(resume_obj, "has_magnums", 0);
        resume->flags.has_uzis = JSON_ObjectGetBool(resume_obj, "has_uzis", 0);
        resume->flags.has_shotgun =
            JSON_ObjectGetBool(resume_obj, "has_shotgun", 0);
        resume->flags.has_m16 = JSON_ObjectGetBool(resume_obj, "has_m16", 0);
        resume->flags.has_grenade =
            JSON_ObjectGetBool(resume_obj, "has_grenade", 0);
        resume->flags.has_harpoon =
            JSON_ObjectGetBool(resume_obj, "has_harpoon", 0);

        resume->stats.timer = JSON_ObjectGetInt(resume_obj, "timer", 0);
        resume->stats.ammo_hits = JSON_ObjectGetInt(resume_obj, "ammo_hits", 0);
        resume->stats.ammo_used = JSON_ObjectGetInt(resume_obj, "ammo_used", 0);
        resume->stats.distance_travelled =
            JSON_ObjectGetInt(resume_obj, "distance_travelled", 0);
        resume->stats.kill_count = JSON_ObjectGetInt(resume_obj, "kills", 0);
        resume->stats.secret_flags =
            JSON_ObjectGetInt(resume_obj, "secrets", 0);
        Stats_UpdateSecrets(&resume->stats);
        resume->stats.medipacks_used =
            JSON_ObjectGetDouble(resume_obj, "medipacks_used", 0);
        resume->stats.max_secret_count =
            JSON_ObjectGetInt(resume_obj, "max_secrets", 0);
        resume->stats.all_secrets_mask =
            JSON_ObjectGetInt(resume_obj, "all_secrets_mask", 0);
    }

    return true;
}

static JSON_OBJECT *M_DumpInventory(void)
{
    JSON_OBJECT *const inv_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(inv_obj, "pickup1", Inv_RequestItem(O_PICKUP_ITEM_1));
    JSON_ObjectAppendInt(inv_obj, "pickup2", Inv_RequestItem(O_PICKUP_ITEM_2));
    JSON_ObjectAppendInt(inv_obj, "puzzle1", Inv_RequestItem(O_PUZZLE_ITEM_1));
    JSON_ObjectAppendInt(inv_obj, "puzzle2", Inv_RequestItem(O_PUZZLE_ITEM_2));
    JSON_ObjectAppendInt(inv_obj, "puzzle3", Inv_RequestItem(O_PUZZLE_ITEM_3));
    JSON_ObjectAppendInt(inv_obj, "puzzle4", Inv_RequestItem(O_PUZZLE_ITEM_4));
    JSON_ObjectAppendInt(inv_obj, "key1", Inv_RequestItem(O_KEY_ITEM_1));
    JSON_ObjectAppendInt(inv_obj, "key2", Inv_RequestItem(O_KEY_ITEM_2));
    JSON_ObjectAppendInt(inv_obj, "key3", Inv_RequestItem(O_KEY_ITEM_3));
    JSON_ObjectAppendInt(inv_obj, "key4", Inv_RequestItem(O_KEY_ITEM_4));
    return inv_obj;
}

static bool M_LoadInventory(JSON_OBJECT *const inv_obj)
{
    if (inv_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing inventory info");
        return false;
    }

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    Lara_InitialiseInventory(current_level);
    Inv_AddItemNTimes(
        O_PICKUP_ITEM_1, JSON_ObjectGetInt(inv_obj, "pickup1", 0));
    Inv_AddItemNTimes(
        O_PICKUP_ITEM_2, JSON_ObjectGetInt(inv_obj, "pickup2", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM_1, JSON_ObjectGetInt(inv_obj, "puzzle1", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM_2, JSON_ObjectGetInt(inv_obj, "puzzle2", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM_3, JSON_ObjectGetInt(inv_obj, "puzzle3", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM_4, JSON_ObjectGetInt(inv_obj, "puzzle4", 0));
    Inv_AddItemNTimes(O_KEY_ITEM_1, JSON_ObjectGetInt(inv_obj, "key1", 0));
    Inv_AddItemNTimes(O_KEY_ITEM_2, JSON_ObjectGetInt(inv_obj, "key2", 0));
    Inv_AddItemNTimes(O_KEY_ITEM_3, JSON_ObjectGetInt(inv_obj, "key3", 0));
    Inv_AddItemNTimes(O_KEY_ITEM_4, JSON_ObjectGetInt(inv_obj, "key4", 0));

    return true;
}

static JSON_OBJECT *M_DumpFlipmaps(void)
{
    JSON_OBJECT *const flipmap_obj = JSON_ObjectNew();
    JSON_ObjectAppendBool(flipmap_obj, "status", Room_GetFlipStatus());
    JSON_ObjectAppendInt(flipmap_obj, "effect", Room_GetFlipEffect());
    JSON_ObjectAppendInt(flipmap_obj, "timer", Room_GetFlipTimer());
    JSON_ARRAY *const flipmap_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        JSON_ArrayAppendInt(flipmap_arr, Room_GetFlipSlotFlags(i) >> 8);
    }
    JSON_ObjectAppendArray(flipmap_obj, "table", flipmap_arr);
    return flipmap_obj;
}

static bool M_LoadFlipmaps(JSON_OBJECT *const flipmap_obj)
{
    if (flipmap_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing flipmap info");
        return false;
    }

    if (JSON_ObjectGetBool(flipmap_obj, "status", false)) {
        Room_FlipMap();
    }

    Room_SetFlipEffect(JSON_ObjectGetInt(flipmap_obj, "effect", 0));
    Room_SetFlipTimer(JSON_ObjectGetInt(flipmap_obj, "timer", 0));

    const JSON_ARRAY *const flipmap_arr =
        JSON_ObjectGetArray(flipmap_obj, "table");
    if (flipmap_arr == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing flipmap table");
        return false;
    }
    if ((signed)flipmap_arr->length != MAX_FLIP_MAPS) {
        LOG_ERROR(
            "Malformed save: expected %d flipmap elements, got %d",
            MAX_FLIP_MAPS, flipmap_arr->length);
        return false;
    }
    for (int32_t i = 0; i < (signed)flipmap_arr->length; i++) {
        Room_SetFlipSlotFlags(i, JSON_ArrayGetInt(flipmap_arr, i, 0) << 8);
    }

    return true;
}

static JSON_ARRAY *M_DumpCameras(void)
{
    JSON_ARRAY *const cameras_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        JSON_ArrayAppendInt(cameras_arr, object->flags);
    }
    return cameras_arr;
}

static bool M_LoadCameras(JSON_ARRAY *const cameras_arr)
{
    if (cameras_arr == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing cameras array");
        return false;
    }

    const int32_t num_cameras = Camera_GetFixedObjectCount();
    if ((signed)cameras_arr->length != num_cameras) {
        LOG_ERROR(
            "Malformed save: expected %d cameras, got %d", num_cameras,
            cameras_arr->length);
        return false;
    }

    for (int32_t i = 0; i < num_cameras; i++) {
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        object->flags = JSON_ArrayGetInt(cameras_arr, i, 0);
    }

    return true;
}

static void M_GetFXOrder(SAVEGAME_BSON_FX_ORDER *const order)
{
    order->count = 0;
    for (int32_t i = 0; i < MAX_EFFECTS; i++) {
        order->id_map[i] = -1;
    }

    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        order->id_map[link_num] = order->count;
        order->count++;
    }
}

static JSON_ARRAY *M_DumpItems(void)
{
    Savegame_ProcessItemsBeforeSave();

    SAVEGAME_BSON_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    JSON_ARRAY *const items_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        JSON_OBJECT *const item_obj = JSON_ObjectNew();
        const ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        JSON_ObjectAppendInt(
            item_obj, "obj_num", Object_ToGameID(item->object_id));

        if (obj->save_position) {
            DUMP_XYZ(item_obj, "pos", item->pos);
            DUMP_XYZ(item_obj, "rot", item->rot);
            JSON_ObjectAppendInt(item_obj, "room_num", item->room_num);
            JSON_ObjectAppendInt(item_obj, "speed", item->speed);
            JSON_ObjectAppendInt(item_obj, "fall_speed", item->fall_speed);
        }

        if (obj->save_anim) {
            JSON_ObjectAppendInt(
                item_obj, "current_anim", item->current_anim_state);
            JSON_ObjectAppendInt(item_obj, "goal_anim", item->goal_anim_state);
            JSON_ObjectAppendInt(
                item_obj, "required_anim", item->required_anim_state);
            JSON_ObjectAppendInt(item_obj, "anim_num", item->anim_num);
            JSON_ObjectAppendInt(item_obj, "frame_num", item->frame_num);
        }

        if (obj->save_hitpoints) {
            JSON_ObjectAppendInt(item_obj, "hitpoints", item->hit_points);
            JSON_ObjectAppendInt(
                item_obj, "max_hitpoints", item->max_hit_points);
        }

        if (obj->save_flags) {
            JSON_ObjectAppendInt(item_obj, "flags", item->flags);
            JSON_ObjectAppendInt(item_obj, "status", item->status);
            JSON_ObjectAppendBool(item_obj, "active", item->active);
            JSON_ObjectAppendBool(item_obj, "gravity", item->gravity);
            JSON_ObjectAppendBool(item_obj, "collidable", item->collidable);
            JSON_ObjectAppendBool(
                item_obj, "intelligent", obj->intelligent && item->data);
            JSON_ObjectAppendInt(item_obj, "timer", item->timer);
            if (obj->intelligent && item->data != nullptr) {
                const CREATURE *const creature = (CREATURE *)item->data;
                JSON_ObjectAppendInt(
                    item_obj, "head_rot", creature->head_rotation);
                JSON_ObjectAppendInt(
                    item_obj, "neck_rot", creature->neck_rotation);
                JSON_ObjectAppendInt(
                    item_obj, "max_turn", creature->maximum_turn);
                JSON_ObjectAppendInt(
                    item_obj, "creature_flags", creature->flags);
                JSON_ObjectAppendInt(item_obj, "creature_mood", creature->mood);
            }

            if (item->object_id == O_FLAME_EMITTER && item->data != nullptr) {
                int32_t effect_num = (int32_t)(intptr_t)item->data - 1;
                effect_num = fx_order.id_map[effect_num];
                JSON_ObjectAppendInt(item_obj, "fx_num", effect_num);
            }

            if (Object_IsType(item->object_id, g_MovableBlockObjects)
                && item->data != nullptr) {
                MOVABLE_BLOCK_INFO *const data = item->data;
                JSON_OBJECT *const data_obj = JSON_ObjectNew();
                JSON_ObjectAppendInt(
                    data_obj, "counter_rot_0", data->counter_rot[0]);
                JSON_ObjectAppendInt(
                    data_obj, "counter_rot_1", data->counter_rot[1]);
                JSON_ObjectAppendInt(
                    data_obj, "counter_rot_2", data->counter_rot[2]);
                JSON_ObjectAppendInt(
                    data_obj, "original_rot", data->original_rot);
                JSON_ObjectAppendInt(
                    data_obj, "gravity_frames", data->gravity_frames);
                JSON_ObjectAppendBool(
                    data_obj, "is_push_pull", data->is_push_pull);
                JSON_ObjectAppendBool(
                    data_obj, "is_forced_moving", data->is_forced_moving);
                DUMP_XYZ(data_obj, "linked", data->linked);
                JSON_ObjectAppendObject(item_obj, "data", data_obj);
            }

            if (item->object_id == O_SLIDING_PILLAR && item->data != nullptr) {
                SLIDING_PILLAR_INFO *const data = item->data;
                JSON_OBJECT *const data_obj = JSON_ObjectNew();
                DUMP_XYZ(data_obj, "linked", data->linked);
                JSON_ObjectAppendObject(item_obj, "data", data_obj);
            }
        }

        JSON_ARRAY *const carried_items_arr = JSON_ArrayNew();

        const CARRIED_ITEM *drop_item = item->carried_item;
        while (drop_item != nullptr) {
            JSON_OBJECT *drop_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(
                drop_obj, "object_id", Object_ToGameID(drop_item->object_id));
            DUMP_XYZ(drop_obj, "pos", drop_item->pos);
            JSON_ObjectAppendInt(drop_obj, "y_rot", drop_item->rot.y);
            JSON_ObjectAppendInt(drop_obj, "room_num", drop_item->room_num);
            JSON_ObjectAppendInt(drop_obj, "fall_speed", drop_item->fall_speed);

            const DROP_STATUS status = Carrier_GetSaveStatus(drop_item);
            JSON_ObjectAppendInt(drop_obj, "status", status);

            JSON_ArrayAppendObject(carried_items_arr, drop_obj);
            drop_item = drop_item->next_item;
        }

        JSON_ObjectAppendArray(item_obj, "carried_items", carried_items_arr);

        switch (item->object_id) {
        case O_BOAT: {
            const BOAT_INFO *const data = (BOAT_INFO *)item->data;
            JSON_OBJECT *const data_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(data_obj, "boat_turn", data->boat_turn);
            JSON_ObjectAppendInt(
                data_obj, "left_fallspeed", data->left_fallspeed);
            JSON_ObjectAppendInt(
                data_obj, "right_fallspeed", data->right_fallspeed);
            JSON_ObjectAppendInt(data_obj, "tilt_angle", data->tilt_angle);
            JSON_ObjectAppendInt(
                data_obj, "extra_rotation", data->extra_rotation);
            JSON_ObjectAppendInt(data_obj, "water", data->water);
            JSON_ObjectAppendInt(data_obj, "pitch", data->pitch);
            JSON_ObjectAppendObject(item_obj, "data", data_obj);
            break;
        }

        case O_SKIDOO_FAST: {
            const SKIDOO_INFO *const data = (SKIDOO_INFO *)item->data;
            JSON_OBJECT *const data_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(data_obj, "track_mesh", data->track_mesh);
            JSON_ObjectAppendInt(data_obj, "skidoo_turn", data->skidoo_turn);
            JSON_ObjectAppendInt(
                data_obj, "left_fallspeed", data->left_fallspeed);
            JSON_ObjectAppendInt(
                data_obj, "right_fallspeed", data->right_fallspeed);
            JSON_ObjectAppendInt(
                data_obj, "momentum_angle", data->momentum_angle);
            JSON_ObjectAppendInt(
                data_obj, "extra_rotation", data->extra_rotation);
            JSON_ObjectAppendInt(data_obj, "pitch", data->pitch);
            JSON_ObjectAppendObject(item_obj, "data", data_obj);
            break;
        }

        case O_LIFT: {
            LIFT_INFO *const data = (LIFT_INFO *)item->data;
            JSON_OBJECT *const data_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(data_obj, "start_height", data->start_height);
            JSON_ObjectAppendInt(data_obj, "wait_time", data->wait_time);
            JSON_ObjectAppendBool(data_obj, "is_moving", data->is_moving);
            for (int32_t j = 0; j < LIFT_NUM_SECTORS; j++) {
                const char *const pos_key = String_FormatStatic("linked_%d", j);
                DUMP_XYZ(data_obj, pos_key, data->linked[j]);
            }
            JSON_ObjectAppendObject(item_obj, "data", data_obj);
            break;
        }

        default:
            break;
        }

        JSON_ArrayAppendObject(items_arr, item_obj);
    }
    return items_arr;
}

static JSON_ARRAY *M_DumpEffects(void)
{
    JSON_ARRAY *const fx_arr = JSON_ArrayNew();

    SAVEGAME_BSON_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        JSON_OBJECT *const fx_obj = JSON_ObjectNew();
        EFFECT *const effect = Effect_Get(link_num);
        DUMP_XYZ(fx_obj, "pos", effect->pos);
        DUMP_XYZ(fx_obj, "rot", effect->rot);
        JSON_ObjectAppendInt(fx_obj, "room_number", effect->room_num);
        JSON_ObjectAppendInt(
            fx_obj, "object_number", Object_ToGameID(effect->object_id));
        JSON_ObjectAppendInt(fx_obj, "speed", effect->speed);
        JSON_ObjectAppendInt(fx_obj, "fall_speed", effect->fall_speed);
        JSON_ObjectAppendInt(fx_obj, "frame_number", effect->frame_num);
        JSON_ObjectAppendInt(fx_obj, "counter", effect->counter);
        JSON_ObjectAppendInt(fx_obj, "shade", effect->shade);
        JSON_ArrayAppendObject(fx_arr, fx_obj);
    }

    return fx_arr;
}

static bool M_IsValidItemObject(
    const OBJECT_ID saved_obj_id, const OBJECT_ID initial_obj_id)
{
    if (saved_obj_id == initial_obj_id) {
        return true;
    }

    // clang-format off
    switch (saved_obj_id) {
        // used keyholes
        case O_PUZZLE_DONE_1: return initial_obj_id == O_PUZZLE_HOLE_1;
        case O_PUZZLE_DONE_2: return initial_obj_id == O_PUZZLE_HOLE_2;
        case O_PUZZLE_DONE_3: return initial_obj_id == O_PUZZLE_HOLE_3;
        case O_PUZZLE_DONE_4: return initial_obj_id == O_PUZZLE_HOLE_4;
        // pickups
        case O_PISTOL_AMMO_ITEM: return initial_obj_id == O_PISTOL_ITEM;
        case O_SHOTGUN_AMMO_ITEM: return initial_obj_id == O_SHOTGUN_ITEM;
        case O_MAGNUM_AMMO_ITEM: return initial_obj_id == O_MAGNUM_ITEM;
        case O_UZI_AMMO_ITEM: return initial_obj_id == O_UZI_ITEM;
        case O_HARPOON_AMMO_ITEM: return initial_obj_id == O_HARPOON_ITEM;
        case O_M16_AMMO_ITEM: return initial_obj_id == O_M16_ITEM;
        case O_GRENADE_AMMO_ITEM: return initial_obj_id == O_GRENADE_ITEM;
        // skidoo swaps
        case O_SKIDOO_FAST: return initial_obj_id == O_SKIDOO_ARMED;
        // default
        default: return false;
    }
    // clang-format on
}

static bool M_LoadItems(JSON_ARRAY *const items_arr, uint16_t header_version)
{
    if (items_arr == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing items array");
        return false;
    }

    const int32_t item_count = Item_GetLevelCount();
    if ((signed)items_arr->length != item_count) {
        LOG_ERROR(
            "Malformed save: expected %d items, got %d", item_count,
            items_arr->length);
        return false;
    }

    Savegame_ProcessItemsBeforeLoad();

    for (int32_t i = 0; i < item_count; i++) {
        JSON_OBJECT *const item_obj = JSON_ArrayGetObject(items_arr, i);
        if (item_obj == nullptr) {
            LOG_ERROR("Malformed save: invalid item data");
            return false;
        }

        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        const OBJECT_ID obj_id =
            Object_FromGameID(JSON_ObjectGetInt(item_obj, "obj_num", -1));
        if (!M_IsValidItemObject(obj_id, item->object_id)) {
            LOG_ERROR(
                "Malformed save: expected object %d, got %d", item->object_id,
                obj_id);
            return false;
        }

        if (obj->save_position) {
            LOAD_XYZ(item_obj, "pos", item->pos);
            LOAD_XYZ(item_obj, "rot", item->rot);
            item->speed = JSON_ObjectGetInt(item_obj, "speed", item->speed);
            item->fall_speed =
                JSON_ObjectGetInt(item_obj, "fall_speed", item->fall_speed);

            int16_t room_num = JSON_ObjectGetInt(item_obj, "room_num", -1);
            Item_UpdateRoom(i, room_num);
        }

        if (obj->save_anim) {
            item->current_anim_state = JSON_ObjectGetInt(
                item_obj, "current_anim", item->current_anim_state);
            item->goal_anim_state =
                JSON_ObjectGetInt(item_obj, "goal_anim", item->goal_anim_state);
            item->required_anim_state = JSON_ObjectGetInt(
                item_obj, "required_anim", item->required_anim_state);
            item->anim_num =
                JSON_ObjectGetInt(item_obj, "anim_num", item->anim_num);
            item->frame_num =
                JSON_ObjectGetInt(item_obj, "frame_num", item->frame_num);

            if (item->object_id == O_LARA
                && item->anim_num < LARA_ORIGINAL_ANIM_COUNT) {
                item->anim_num += obj->anim_idx;
            }
        }

        if (obj->save_hitpoints) {
            item->hit_points =
                JSON_ObjectGetInt(item_obj, "hitpoints", item->hit_points);
            item->max_hit_points = JSON_ObjectGetInt(
                item_obj, "max_hitpoints", item->max_hit_points);
        }

        if (obj->save_flags) {
            item->flags = JSON_ObjectGetInt(item_obj, "flags", item->flags);
            item->timer = JSON_ObjectGetInt(item_obj, "timer", item->timer);

            if ((item->flags & IF_KILLED) != 0) {
                Item_Kill(i);
                item->status = IS_DEACTIVATED;
            } else {
                if (JSON_ObjectGetBool(item_obj, "active", item->active)
                    && !item->active) {
                    Item_AddActive(i);
                }
                item->status =
                    JSON_ObjectGetInt(item_obj, "status", item->status);
                item->gravity =
                    JSON_ObjectGetBool(item_obj, "gravity", item->gravity);
                item->collidable = JSON_ObjectGetBool(
                    item_obj, "collidable", item->collidable);
            }

            if (JSON_ObjectGetBool(item_obj, "intelligent", obj->intelligent)) {
                LOT_EnableBaddieAI(i, true);
                CREATURE *const creature = (CREATURE *)item->data;
                if (creature != nullptr) {
                    creature->head_rotation = JSON_ObjectGetInt(
                        item_obj, "head_rot", creature->head_rotation);
                    creature->neck_rotation = JSON_ObjectGetInt(
                        item_obj, "neck_rot", creature->neck_rotation);
                    creature->maximum_turn = JSON_ObjectGetInt(
                        item_obj, "max_turn", creature->maximum_turn);
                    creature->flags = JSON_ObjectGetInt(
                        item_obj, "creature_flags", creature->flags);
                    creature->mood = JSON_ObjectGetInt(
                        item_obj, "creature_mood", creature->mood);
                }
            } else if (obj->intelligent) {
                item->data = nullptr;
                if (item->killed && item->hit_points <= 0
                    && !(item->flags & IF_KILLED)) {
                    item->next_active = Item_GetPrevActive();
                    Item_SetPrevActive(i);
                }
            }

            if (item->object_id == O_FLAME_EMITTER
                && g_Config.gameplay.enable_enhanced_saves) {
                const int32_t effect_num =
                    JSON_ObjectGetInt(item_obj, "fx_num", NO_EFFECT);
                if (effect_num != NO_EFFECT) {
                    item->data = (void *)(intptr_t)(effect_num + 1);
                }
            }

            if (header_version >= VERSION_12
                && Object_IsType(item->object_id, g_MovableBlockObjects)) {
                JSON_OBJECT *const data_obj =
                    JSON_ObjectGetObject(item_obj, "data");
                if (data_obj == nullptr) {
                    LOG_ERROR(
                        "Malformed save: missing movable block data for item "
                        "%d",
                        i);
                    return false;
                }
                MOVABLE_BLOCK_INFO *const data = item->data;
                data->counter_rot[0] = JSON_ObjectGetInt(
                    data_obj, "counter_rot_0", data->counter_rot[0]);
                data->counter_rot[1] = JSON_ObjectGetInt(
                    data_obj, "counter_rot_1", data->counter_rot[1]);
                data->counter_rot[2] = JSON_ObjectGetInt(
                    data_obj, "counter_rot_2", data->counter_rot[2]);
                data->original_rot = JSON_ObjectGetInt(
                    data_obj, "original_rot", data->original_rot);
                data->gravity_frames = JSON_ObjectGetInt(
                    data_obj, "gravity_frames", data->gravity_frames);
                data->is_push_pull = JSON_ObjectGetBool(
                    data_obj, "is_push_pull", data->is_push_pull);
                data->is_forced_moving = JSON_ObjectGetBool(
                    data_obj, "is_forced_moving", data->is_forced_moving);
                LOAD_XYZ(data_obj, "linked", data->linked);
            } else if (Object_IsType(item->object_id, g_MovableBlockObjects)) {
                // For old saves, guess linked sector is at item position.
                MOVABLE_BLOCK_INFO *const data = item->data;
                data->linked.pos = item->pos;
                data->linked.room_num = item->room_num;
            }

            if (header_version >= VERSION_12
                && item->object_id == O_SLIDING_PILLAR
                && item->data != nullptr) {
                JSON_OBJECT *const data_obj =
                    JSON_ObjectGetObject(item_obj, "data");
                if (data_obj == nullptr) {
                    LOG_ERROR(
                        "Malformed save: missing sliding pillar data for item "
                        "%d",
                        i);
                    return false;
                }
                SLIDING_PILLAR_INFO *const data = item->data;
                LOAD_XYZ(data_obj, "linked", data->linked);
            } else if (item->object_id == O_SLIDING_PILLAR) {
                // For old saves, guess linked sector is at item position.
                SLIDING_PILLAR_INFO *const data = item->data;
                data->linked.pos = item->pos;
                data->linked.room_num = item->room_num;
            }

            JSON_ARRAY *const carried_items =
                JSON_ObjectGetArray(item_obj, "carried_items");
            if (carried_items != nullptr) {
                CARRIED_ITEM *carried_item = item->carried_item;
                for (int32_t j = 0; j < (signed)carried_items->length; j++) {
                    if (carried_item == nullptr) {
                        LOG_ERROR("Malformed save: carried item mismatch");
                        return false;
                    }

                    JSON_OBJECT *const carried_item_obj =
                        JSON_ArrayGetObject(carried_items, j);
                    carried_item->object_id =
                        Object_FromGameID(JSON_ObjectGetInt(
                            carried_item_obj, "object_id",
                            carried_item->object_id));
                    LOAD_XYZ(carried_item_obj, "pos", carried_item->pos);
                    carried_item->rot.y = JSON_ObjectGetInt(
                        carried_item_obj, "y_rot", carried_item->rot.y);
                    carried_item->room_num = JSON_ObjectGetInt(
                        carried_item_obj, "room_num", carried_item->room_num);
                    carried_item->fall_speed = JSON_ObjectGetInt(
                        carried_item_obj, "fall_speed",
                        carried_item->fall_speed);
                    carried_item->status = JSON_ObjectGetInt(
                        carried_item_obj, "status", carried_item->status);

                    carried_item = carried_item->next_item;
                }

                Carrier_TestItemDrops(i);
            }

            switch (item->object_id) {
            case O_BOAT: {
                const JSON_OBJECT *const data_obj =
                    JSON_ObjectGetObject(item_obj, "data");
                if (data_obj == nullptr) {
                    LOG_ERROR(
                        "Malformed save: missing boat data for item %d", i);
                    return false;
                }
                BOAT_INFO *const data = (BOAT_INFO *)item->data;
                data->boat_turn =
                    JSON_ObjectGetInt(data_obj, "boat_turn", data->boat_turn);
                data->left_fallspeed = JSON_ObjectGetInt(
                    data_obj, "left_fallspeed", data->left_fallspeed);
                data->right_fallspeed = JSON_ObjectGetInt(
                    data_obj, "right_fallspeed", data->right_fallspeed);
                data->tilt_angle =
                    JSON_ObjectGetInt(data_obj, "tilt_angle", data->tilt_angle);
                data->extra_rotation = JSON_ObjectGetInt(
                    data_obj, "extra_rotation", data->extra_rotation);
                data->water = JSON_ObjectGetInt(data_obj, "water", data->water);
                data->pitch = JSON_ObjectGetInt(data_obj, "pitch", data->pitch);
                break;
            }

            case O_SKIDOO_FAST: {
                const JSON_OBJECT *const data_obj =
                    JSON_ObjectGetObject(item_obj, "data");
                if (data_obj == nullptr) {
                    LOG_ERROR(
                        "Malformed save: missing skidoo data for item %d", i);
                    return false;
                }
                SKIDOO_INFO *const data = (SKIDOO_INFO *)item->data;
                data->track_mesh =
                    JSON_ObjectGetInt(data_obj, "track_mesh", data->track_mesh);
                data->skidoo_turn = JSON_ObjectGetInt(
                    data_obj, "skidoo_turn", data->skidoo_turn);
                data->left_fallspeed = JSON_ObjectGetInt(
                    data_obj, "left_fallspeed", data->left_fallspeed);
                data->right_fallspeed = JSON_ObjectGetInt(
                    data_obj, "right_fallspeed", data->right_fallspeed);
                data->momentum_angle = JSON_ObjectGetInt(
                    data_obj, "momentum_angle", data->momentum_angle);
                data->extra_rotation = JSON_ObjectGetInt(
                    data_obj, "extra_rotation", data->extra_rotation);
                data->pitch = JSON_ObjectGetInt(data_obj, "pitch", data->pitch);
                break;
            }

            case O_LIFT: {
                JSON_OBJECT *const data_obj =
                    JSON_ObjectGetObject(item_obj, "data");
                if (data_obj == nullptr) {
                    LOG_ERROR(
                        "Malformed save: missing lift data for item %d", i);
                    return false;
                }
                LIFT_INFO *const data = (LIFT_INFO *)item->data;
                data->start_height = JSON_ObjectGetInt(
                    data_obj, "start_height", data->start_height);
                data->wait_time =
                    JSON_ObjectGetInt(data_obj, "wait_time", data->wait_time);
                if (header_version >= VERSION_12) {
                    data->is_moving = JSON_ObjectGetBool(
                        data_obj, "is_moving", data->is_moving);
                    for (int32_t j = 0; j < LIFT_NUM_SECTORS; j++) {
                        const char *const pos_key =
                            String_FormatStatic("linked_%d", j);
                        LOAD_XYZ(data_obj, pos_key, data->linked[j]);
                    }
                }
                break;
            }

            default:
                break;
            }

            if (obj->handle_save_func != nullptr) {
                obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
            }
        }
    }

    return true;
}

static bool M_LoadEffects(JSON_ARRAY *const fx_arr)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return true;
    }

    if (fx_arr == nullptr) {
        // ..1.1 saves do not contain effects.
        return true;
    }

    if ((signed)fx_arr->length >= MAX_EFFECTS) {
        LOG_WARNING(
            "Malformed save: expected a max of %d effect, got %d. effect over "
            "the maximum will not be created.",
            MAX_EFFECTS - 1, fx_arr->length);
    }

    for (int i = 0; i < (signed)fx_arr->length; i++) {
        JSON_OBJECT *const fx_obj = JSON_ArrayGetObject(fx_arr, i);
        if (fx_obj == nullptr) {
            LOG_ERROR("Malformed save: invalid effect data");
            return false;
        }

        const int16_t room_num =
            JSON_ObjectGetInt(fx_obj, "room_number", NO_ROOM);
        ASSERT(room_num != NO_ROOM);
        const int16_t effect_num = Effect_Create(room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            LOAD_XYZ(fx_obj, "pos", effect->pos);
            LOAD_XYZ(fx_obj, "rot", effect->rot);
            effect->room_num = room_num;
            effect->object_id = Object_FromGameID(
                JSON_ObjectGetInt(fx_obj, "object_number", NO_OBJECT));
            effect->speed = JSON_ObjectGetInt(fx_obj, "speed", 0);
            effect->fall_speed = JSON_ObjectGetInt(fx_obj, "fall_speed", 0);
            effect->frame_num = JSON_ObjectGetInt(fx_obj, "frame_number", 0);
            effect->counter = JSON_ObjectGetInt(fx_obj, "counter", 0);
            effect->shade = JSON_ObjectGetInt(fx_obj, "shade", 0);
        }
    }

    return true;
}

static JSON_ARRAY *M_DumpFlares(void)
{
    JSON_ARRAY *const flares_arr = JSON_ArrayNew();
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (!item->active || item->object_id != O_FLARE_ITEM) {
            continue;
        }

        JSON_OBJECT *const flare_obj = JSON_ObjectNew();
        DUMP_XYZ(flare_obj, "pos", item->pos);
        DUMP_XYZ(flare_obj, "rot", item->rot);
        JSON_ObjectAppendInt(flare_obj, "room_num", item->room_num);
        JSON_ObjectAppendInt(flare_obj, "speed", item->speed);
        JSON_ObjectAppendInt(flare_obj, "fall_speed", item->fall_speed);
        JSON_ObjectAppendInt(flare_obj, "age", (intptr_t)item->data);
        JSON_ArrayAppendObject(flares_arr, flare_obj);
    }
    return flares_arr;
}

static bool M_LoadFlares(JSON_ARRAY *const flares_arr)
{
    if (flares_arr == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing flares array");
        return false;
    }

    for (int32_t i = 0; i < (signed)flares_arr->length; i++) {
        JSON_OBJECT *const flare_obj = JSON_ArrayGetObject(flares_arr, i);
        if (flare_obj == nullptr) {
            LOG_ERROR("Malformed save: invalid flare data");
            return false;
        }

        const int16_t item_num = Item_Create();
        ITEM *const item = Item_Get(item_num);
        item->object_id = O_FLARE_ITEM;
        LOAD_XYZ(flare_obj, "pos", item->pos);
        LOAD_XYZ(flare_obj, "rot", item->rot);
        item->room_num =
            JSON_ObjectGetInt(flare_obj, "room_num", item->room_num);
        item->speed = JSON_ObjectGetInt(flare_obj, "speed", item->speed);
        item->fall_speed =
            JSON_ObjectGetInt(flare_obj, "fall_speed", item->fall_speed);
        Item_Initialise(item_num);
        Item_AddActive(item_num);
        const int32_t flare_age = JSON_ObjectGetInt(flare_obj, "age", 0);
        item->data = (void *)(intptr_t)flare_age;
    }
    return true;
}

static JSON_OBJECT *M_DumpArm(const LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    JSON_OBJECT *const arm_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(arm_obj, "anim_num", arm->anim_num);
    JSON_ObjectAppendInt(arm_obj, "frame_num", arm->frame_num);
    JSON_ObjectAppendInt(arm_obj, "lock", arm->lock);
    JSON_ObjectAppendInt(arm_obj, "flash_gun", arm->flash_gun);
    DUMP_XYZ(arm_obj, "rot", arm->rot);
    return arm_obj;
}

static JSON_OBJECT *M_DumpAmmo(const AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    JSON_OBJECT *const ammo_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(ammo_obj, "ammo", ammo->ammo);
    return ammo_obj;
}

static JSON_OBJECT *M_DumpLara(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    JSON_OBJECT *const lara_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(lara_obj, "item_number", lara->item_num);
    JSON_ObjectAppendInt(lara_obj, "gun_status", lara->gun_status);
    JSON_ObjectAppendInt(lara_obj, "gun_type", lara->gun_type);
    JSON_ObjectAppendInt(lara_obj, "request_gun_type", lara->request_gun_type);
    JSON_ObjectAppendInt(lara_obj, "last_gun_type", lara->last_gun_type);
    JSON_ObjectAppendInt(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    JSON_ObjectAppendInt(lara_obj, "water_status", lara->water_status);
    JSON_ObjectAppendInt(lara_obj, "climb_status", lara->climb_status);
    JSON_ObjectAppendInt(lara_obj, "pose_count", lara->pose_count);
    JSON_ObjectAppendInt(lara_obj, "hit_frame", lara->hit_frame);
    JSON_ObjectAppendInt(lara_obj, "hit_direction", lara->hit_direction);
    JSON_ObjectAppendInt(lara_obj, "air", lara->air);
    JSON_ObjectAppendInt(lara_obj, "sprint_timer", lara->sprint_timer);
    JSON_ObjectAppendInt(lara_obj, "exposure_timer", lara->exposure_timer);
    JSON_ObjectAppendInt(lara_obj, "dive_count", lara->dive_timer);
    JSON_ObjectAppendInt(lara_obj, "death_count", lara->death_timer);
    JSON_ObjectAppendInt(lara_obj, "current_active", lara->current_active);
    JSON_ObjectAppendInt(lara_obj, "hit_effect_count", lara->hit_effect_count);
    JSON_ObjectAppendInt(lara_obj, "flare_age", lara->flare.age);
    JSON_ObjectAppendInt(
        lara_obj, "vehicle_item_number", Lara_Vehicle_GetIndex());
    JSON_ObjectAppendInt(
        lara_obj, "back_gun_obj_id", Object_ToGameID(lara->back_gun_obj_id));
    JSON_ObjectAppendInt(lara_obj, "flare_frame", lara->flare.frame_num);
    JSON_ObjectAppendInt(lara_obj, "mesh_effects", lara->mesh_effects);
    JSON_ObjectAppendInt(
        lara_obj, "water_surface_dist", lara->water_surface_dist);

    JSON_ObjectAppendBool(lara_obj, "flare_control_left", lara->flare.control);
    JSON_ObjectAppendBool(lara_obj, "extra_anim", lara->extra_anim);
    JSON_ObjectAppendBool(lara_obj, "burn", lara->burn);

    JSON_ARRAY *const lara_meshes_arr = JSON_ArrayNew();
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        JSON_ArrayAppendInt(
            lara_meshes_arr, Object_GetMeshOffset(lara->mesh_ptrs[i]));
    }
    JSON_ObjectAppendArray(lara_obj, "meshes", lara_meshes_arr);

    JSON_ObjectAppendInt(lara_obj, "target_angle1", lara->target_angles[0]);
    JSON_ObjectAppendInt(lara_obj, "target_angle2", lara->target_angles[1]);
    JSON_ObjectAppendInt(lara_obj, "turn_rate", lara->turn_rate);
    JSON_ObjectAppendInt(lara_obj, "move_angle", lara->move_angle);
    DUMP_XYZ(lara_obj, "head_rot", lara->head_rot);
    DUMP_XYZ(lara_obj, "torso_rot", lara->torso_rot);
    DUMP_XYZ(lara_obj, "last_pos", lara->last_pos);

    JSON_ObjectAppendObject(lara_obj, "left_arm", M_DumpArm(&lara->left_arm));
    JSON_ObjectAppendObject(lara_obj, "right_arm", M_DumpArm(&lara->right_arm));
    JSON_ObjectAppendObject(
        lara_obj, "pistols", M_DumpAmmo(&lara->pistol_ammo));
    JSON_ObjectAppendObject(
        lara_obj, "magnums", M_DumpAmmo(&lara->magnum_ammo));
    JSON_ObjectAppendObject(lara_obj, "uzis", M_DumpAmmo(&lara->uzi_ammo));
    JSON_ObjectAppendObject(
        lara_obj, "shotgun", M_DumpAmmo(&lara->shotgun_ammo));
    JSON_ObjectAppendObject(
        lara_obj, "harpoon", M_DumpAmmo(&lara->harpoon_ammo));
    JSON_ObjectAppendObject(
        lara_obj, "grenade", M_DumpAmmo(&lara->grenade_ammo));
    JSON_ObjectAppendObject(lara_obj, "m16", M_DumpAmmo(&lara->m16_ammo));

    if (lara->gun_item_num != NO_ITEM) {
        JSON_OBJECT *const weapon_obj = JSON_ObjectNew();
        const ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        JSON_ObjectAppendInt(
            weapon_obj, "obj_id", Object_ToGameID(weapon_item->object_id));
        JSON_ObjectAppendInt(weapon_obj, "anim_num", weapon_item->anim_num);
        JSON_ObjectAppendInt(weapon_obj, "frame_num", weapon_item->frame_num);
        JSON_ObjectAppendInt(
            weapon_obj, "current_anim_state", weapon_item->current_anim_state);
        JSON_ObjectAppendInt(
            weapon_obj, "goal_anim_state", weapon_item->goal_anim_state);
        JSON_ObjectAppendObject(lara_obj, "weapon", weapon_obj);
    }

    JSON_ObjectAppendInt(
        lara_obj, "interact_target.item_num", lara->interact_target.item_num);
    JSON_ObjectAppendInt(
        lara_obj, "interact_target.move_count",
        lara->interact_target.move_count);
    JSON_ObjectAppendBool(
        lara_obj, "interact_target.is_moving", lara->interact_target.is_moving);

    return lara_obj;
}

static bool M_LoadArm(JSON_OBJECT *const arm_obj, LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    if (arm_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing arm info");
        return false;
    }

    arm->anim_num = JSON_ObjectGetInt(arm_obj, "anim_num", arm->anim_num);
    arm->frame_num = JSON_ObjectGetInt(arm_obj, "frame_num", arm->frame_num);
    arm->lock = JSON_ObjectGetInt(arm_obj, "lock", arm->lock);
    arm->flash_gun = JSON_ObjectGetInt(arm_obj, "flash_gun", arm->flash_gun);
    LOAD_XYZ(arm_obj, "rot", arm->rot);

    return true;
}

static bool M_LoadAmmo(JSON_OBJECT *const ammo_obj, AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    if (ammo_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing ammo info");
        return false;
    }

    ammo->ammo = JSON_ObjectGetInt(ammo_obj, "ammo", ammo->ammo);
    return true;
}

static bool M_LoadLara(JSON_OBJECT *const lara_obj)
{
    if (lara_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing Lara info");
        return false;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    lara->item_num = JSON_ObjectGetInt(lara_obj, "item_number", lara->item_num);
    lara->gun_status =
        JSON_ObjectGetInt(lara_obj, "gun_status", lara->gun_status);
    lara->gun_type = JSON_ObjectGetInt(lara_obj, "gun_type", lara->gun_type);
    lara->request_gun_type =
        JSON_ObjectGetInt(lara_obj, "request_gun_type", lara->request_gun_type);
    lara->last_gun_type =
        JSON_ObjectGetInt(lara_obj, "last_gun_type", lara->last_gun_type);
    lara->calc_fall_speed =
        JSON_ObjectGetInt(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    lara->water_status =
        JSON_ObjectGetInt(lara_obj, "water_status", lara->water_status);
    lara->climb_status =
        JSON_ObjectGetInt(lara_obj, "climb_status", lara->climb_status);
    lara->pose_count =
        JSON_ObjectGetInt(lara_obj, "pose_count", lara->pose_count);
    lara->hit_frame = JSON_ObjectGetInt(lara_obj, "hit_frame", lara->hit_frame);
    lara->hit_direction =
        JSON_ObjectGetInt(lara_obj, "hit_direction", lara->hit_direction);
    lara->air = JSON_ObjectGetInt(lara_obj, "air", lara->air);
    lara->sprint_timer =
        JSON_ObjectGetInt(lara_obj, "sprint_timer", lara->sprint_timer);
    lara->exposure_timer =
        JSON_ObjectGetInt(lara_obj, "exposure_timer", lara->exposure_timer);
    lara->dive_timer =
        JSON_ObjectGetInt(lara_obj, "dive_count", lara->dive_timer);
    lara->death_timer =
        JSON_ObjectGetInt(lara_obj, "death_count", lara->death_timer);
    lara->current_active =
        JSON_ObjectGetInt(lara_obj, "current_active", lara->current_active);
    lara->hit_effect_count =
        JSON_ObjectGetInt(lara_obj, "hit_effect_count", lara->hit_effect_count);
    lara->flare.age = JSON_ObjectGetInt(lara_obj, "flare_age", lara->flare.age);
    lara->back_gun_obj_id = Object_FromGameID(
        JSON_ObjectGetInt(lara_obj, "back_gun_obj_id", lara->back_gun_obj_id));
    lara->flare.frame_num =
        JSON_ObjectGetInt(lara_obj, "flare_frame", lara->flare.frame_num);
    lara->mesh_effects =
        JSON_ObjectGetInt(lara_obj, "mesh_effects", lara->mesh_effects);
    lara->water_surface_dist = JSON_ObjectGetInt(
        lara_obj, "water_surface_dist", lara->water_surface_dist);

    const int16_t vehicle_index = JSON_ObjectGetInt(
        lara_obj, "vehicle_item_number", Lara_Vehicle_GetIndex());
    Lara_Vehicle_SetIndex(vehicle_index);

    lara->flare.control =
        JSON_ObjectGetBool(lara_obj, "flare_control_left", lara->flare.control);
    lara->extra_anim =
        JSON_ObjectGetBool(lara_obj, "extra_anim", lara->extra_anim);
    lara->burn = JSON_ObjectGetBool(lara_obj, "burn", lara->burn);

    const JSON_ARRAY *const lara_meshes_arr =
        JSON_ObjectGetArray(lara_obj, "meshes");
    if (!lara_meshes_arr) {
        LOG_ERROR("Malformed save: invalid or missing Lara meshes");
        return false;
    }
    if ((signed)lara_meshes_arr->length != LM_NUMBER_OF) {
        LOG_ERROR(
            "Malformed save: expected %d Lara meshes, got %d", LM_NUMBER_OF,
            lara_meshes_arr->length);
        return false;
    }

    for (int32_t i = 0; i < (signed)lara_meshes_arr->length; i++) {
        int32_t idx = Object_GetMeshOffset(lara->mesh_ptrs[i]);
        idx = JSON_ArrayGetInt(lara_meshes_arr, i, idx);
        OBJECT_MESH *const mesh = Object_FindMesh(idx);
        if (mesh != nullptr) {
            lara->mesh_ptrs[i] = mesh;
        }
    }

    lara->target = nullptr;
    lara->target_angles[0] =
        JSON_ObjectGetInt(lara_obj, "target_angle1", lara->target_angles[0]);
    lara->target_angles[1] =
        JSON_ObjectGetInt(lara_obj, "target_angle2", lara->target_angles[1]);
    lara->turn_rate = JSON_ObjectGetInt(lara_obj, "turn_rate", lara->turn_rate);
    lara->move_angle =
        JSON_ObjectGetInt(lara_obj, "move_angle", lara->move_angle);
    LOAD_XYZ(lara_obj, "head_rot", lara->head_rot);
    LOAD_XYZ(lara_obj, "torso_rot", lara->torso_rot);
    LOAD_XYZ(lara_obj, "last_pos", lara->last_pos);

    if (!M_LoadArm(
            JSON_ObjectGetObject(lara_obj, "left_arm"), &lara->left_arm)) {
        return false;
    }

    if (!M_LoadArm(
            JSON_ObjectGetObject(lara_obj, "right_arm"), &lara->right_arm)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "pistols"), &lara->pistol_ammo)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "magnums"), &lara->magnum_ammo)) {
        return false;
    }

    if (!M_LoadAmmo(JSON_ObjectGetObject(lara_obj, "uzis"), &lara->uzi_ammo)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "shotgun"), &lara->shotgun_ammo)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "harpoon"), &lara->harpoon_ammo)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "grenade"), &lara->grenade_ammo)) {
        return false;
    }

    if (!M_LoadAmmo(JSON_ObjectGetObject(lara_obj, "m16"), &lara->m16_ammo)) {
        return false;
    }

    const JSON_OBJECT *const weapon_obj =
        JSON_ObjectGetObject(lara_obj, "weapon");
    if (weapon_obj != nullptr) {
        lara->gun_item_num = Item_Create();
        ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        weapon_item->object_id = Object_FromGameID(
            JSON_ObjectGetInt(weapon_obj, "obj_id", weapon_item->object_id));
        weapon_item->anim_num =
            JSON_ObjectGetInt(weapon_obj, "anim_num", weapon_item->anim_num);
        weapon_item->frame_num =
            JSON_ObjectGetInt(weapon_obj, "frame_num", weapon_item->frame_num);
        weapon_item->current_anim_state = JSON_ObjectGetInt(
            weapon_obj, "current_anim_state", weapon_item->current_anim_state);
        weapon_item->goal_anim_state = JSON_ObjectGetInt(
            weapon_obj, "goal_anim_state", weapon_item->goal_anim_state);
        weapon_item->status = IS_ACTIVE;
        weapon_item->room_num = NO_ROOM;
    }

    lara->interact_target.item_num = JSON_ObjectGetInt(
        lara_obj, "interact_target.item_num", lara->interact_target.item_num);
    lara->interact_target.move_count = JSON_ObjectGetInt(
        lara_obj, "interact_target.move_count",
        lara->interact_target.move_count);
    lara->interact_target.is_moving = JSON_ObjectGetBool(
        lara_obj, "interact_target.is_moving", lara->interact_target.is_moving);

    return true;
}

static bool M_LoadFromFile(MYFILE *const fp)
{
    bool result = false;

    int32_t version = -1;
    JSON_VALUE *const root = M_ReadRaw(fp, &version);
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!M_LoadMisc(JSON_ObjectGetObject(root_obj, "misc"))) {
        goto cleanup;
    }

    if (!M_LoadMusic(JSON_ObjectGetObject(root_obj, "music"), version)) {
        goto cleanup;
    }

    if (!M_LoadResumeInfo(JSON_ObjectGetArray(root_obj, "resume_info"))) {
        goto cleanup;
    }

    if (!M_LoadInventory(JSON_ObjectGetObject(root_obj, "inventory"))) {
        goto cleanup;
    }

    if (!M_LoadFlipmaps(JSON_ObjectGetObject(root_obj, "flipmap"))) {
        goto cleanup;
    }

    if (!M_LoadCameras(JSON_ObjectGetArray(root_obj, "cameras"))) {
        goto cleanup;
    }

    if (!M_LoadItems(JSON_ObjectGetArray(root_obj, "items"), version)) {
        goto cleanup;
    }

    if (!M_LoadEffects(JSON_ObjectGetArray(root_obj, "fx"))) {
        goto cleanup;
    }

    if (!M_LoadFlares(JSON_ObjectGetArray(root_obj, "flares"))) {
        goto cleanup;
    }

    if (!M_LoadLara(JSON_ObjectGetObject(root_obj, "lara"))) {
        goto cleanup;
    }

    result = true;

cleanup:
    JSON_ValueFree(root);
    return result;
}

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    JSON_OBJECT *const root_obj = JSON_ObjectNew();

    JSON_ObjectAppendObject(root_obj, "misc", M_DumpMisc());
    JSON_ObjectAppendObject(root_obj, "music", M_DumpMusic());
    JSON_ObjectAppendArray(root_obj, "resume_info", M_DumpResumeInfo());
    JSON_ObjectAppendObject(root_obj, "inventory", M_DumpInventory());
    JSON_ObjectAppendObject(root_obj, "flipmap", M_DumpFlipmaps());
    JSON_ObjectAppendArray(root_obj, "cameras", M_DumpCameras());
    JSON_ObjectAppendArray(root_obj, "items", M_DumpItems());
    JSON_ObjectAppendArray(root_obj, "fx", M_DumpEffects());
    JSON_ObjectAppendArray(root_obj, "flares", M_DumpFlares());
    JSON_ObjectAppendObject(root_obj, "lara", M_DumpLara());

    JSON_VALUE *const root = JSON_ValueFromObject(root_obj);
    M_SaveRaw(fp, root, current_level->num);
    JSON_ValueFree(root);
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
