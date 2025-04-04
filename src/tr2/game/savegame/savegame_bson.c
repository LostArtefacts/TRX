#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/objects/general/lift.h"
#include "game/savegame.h"
#include "global/vars.h"

#include <libtrx/bson.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/music.h>
#include <libtrx/game/shell.h>
#include <libtrx/json.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/version.h>

#include <string.h>
#include <zlib.h>

#define SAVEGAME_BSON_MAGIC MKTAG('T', '2', 'X', 'B')

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

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    int16_t initial_version;
    uint16_t version;
    uint32_t flags;
    int32_t counter;
    int32_t level_num;
    size_t title_size;
    size_t game_version_size;
    size_t compressed_size;
    size_t uncompressed_size;
} SAVEGAME_BSON_HEADER;
#pragma pack(pop)

static void M_SaveRaw(MYFILE *fp, const JSON_VALUE *root);
static JSON_VALUE *M_ReadRaw(MYFILE *fp, int32_t *version_out);
static JSON_VALUE *M_ParseFromBuffer(const char *buffer, int32_t *version_out);

static JSON_OBJECT *M_DumpMisc(void);
static JSON_OBJECT *M_DumpMusic(void);
static JSON_ARRAY *M_DumpResumeInfo(void);
static JSON_OBJECT *M_DumpInventory(void);
static JSON_OBJECT *M_DumpFlipmaps(void);
static JSON_ARRAY *M_DumpCameras(void);
static JSON_ARRAY *M_DumpItems(void);
static JSON_ARRAY *M_DumpFlares(void);
static JSON_OBJECT *M_DumpLara(void);
static JSON_OBJECT *M_DumpArm(const LARA_ARM *arm);
static JSON_OBJECT *M_DumpAmmo(const AMMO_INFO *ammo);

static bool M_LoadMisc(JSON_OBJECT *misc_obj);
static bool M_LoadMusic(JSON_OBJECT *music_obj);
static bool M_LoadResumeInfo(JSON_ARRAY *resume_arr);
static bool M_LoadInventory(JSON_OBJECT *inv_obj);
static bool M_LoadFlipmaps(JSON_OBJECT *flipmap_obj);
static bool M_LoadCameras(JSON_ARRAY *cameras_arr);
static bool M_LoadItems(JSON_ARRAY *items_arr);
static bool M_LoadFlares(JSON_ARRAY *flares_arr);
static bool M_LoadLara(JSON_OBJECT *lara_obj);
static bool M_LoadArm(JSON_OBJECT *arm_obj, LARA_ARM *arm);
static bool M_LoadAmmo(JSON_OBJECT *ammo_obj, AMMO_INFO *ammo);

static bool M_IsValidItemObject(
    GAME_OBJECT_ID saved_obj_id, GAME_OBJECT_ID initial_obj_id);

static const char *M_GetSaveFilePattern(void);
static bool M_FillInfo(MYFILE *fp, SAVEGAME_INFO *info);
static void M_SaveToFile(MYFILE *fp);
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
    info->counter = header.counter;
    info->level_num = header.level_num;
    info->level_title = Memory_Alloc(header.title_size + 1);
    File_ReadData(fp, info->level_title, header.title_size);

    return true;
}

static void M_SaveToFile(MYFILE *const fp)
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
    JSON_ObjectAppendArray(root_obj, "flares", M_DumpFlares());
    JSON_ObjectAppendObject(root_obj, "lara", M_DumpLara());

    JSON_VALUE *const root = JSON_ValueFromObject(root_obj);
    M_SaveRaw(fp, root);
    JSON_ValueFree(root);
}

static void M_SaveRaw(MYFILE *const fp, const JSON_VALUE *const root)
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

    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const SAVEGAME_BSON_HEADER header = {
        .magic = SAVEGAME_BSON_MAGIC,
        .initial_version = Savegame_GetInitialVersion(),
        .version = SAVEGAME_CURRENT_VERSION,
        .flags = Game_GetBonusFlag(),
        .counter = Savegame_GetCounter(),
        .level_num = level->num,
        .title_size = strlen(level->title),
        .game_version_size = strlen(g_TRXVersion),
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };

    File_WriteData(fp, &header, sizeof(header));
    File_WriteData(fp, level->title, strlen(level->title));
    File_WriteData(fp, compressed, compressed_size);
    File_WriteData(fp, g_TRXVersion, strlen(g_TRXVersion));

    Memory_FreePointer(&uncompressed);
    Memory_FreePointer(&compressed);
}

static bool M_LoadFromFile(MYFILE *const fp)
{
    bool result = false;

    int32_t version;
    JSON_VALUE *const root = M_ReadRaw(fp, &version);
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!M_LoadMisc(JSON_ObjectGetObject(root_obj, "misc"))) {
        goto cleanup;
    }

    if (!M_LoadMusic(JSON_ObjectGetObject(root_obj, "music"))) {
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

    if (!M_LoadItems(JSON_ObjectGetArray(root_obj, "items"))) {
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

    const char *compressed =
        buffer + sizeof(SAVEGAME_BSON_HEADER) + header->title_size;
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

static JSON_OBJECT *M_DumpMisc(void)
{
    JSON_OBJECT *const misc_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(misc_obj, "bonus_flag", Game_GetBonusFlag());
    JSON_ObjectAppendBool(misc_obj, "are_monks_angry", g_IsMonkAngry);
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
    g_IsMonkAngry = JSON_ObjectGetBool(misc_obj, "are_monks_angry", 0);
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
    return music_obj;
}

static bool M_LoadMusic(JSON_OBJECT *const music_obj)
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

    if ((signed)track_arr->length != MAX_MUSIC_TRACKS) {
        LOG_WARNING(
            "Malformed save: expected %d music track flags, got %d",
            MAX_MUSIC_TRACKS, track_arr->length);
        return true;
    }

    for (int32_t i = 0; i < (signed)track_arr->length; i++) {
        Music_SetTrackFlags(i, JSON_ArrayGetInt(track_arr, i, 0));
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
        JSON_ObjectAppendInt(resume_obj, "gun_type", resume->gun_type);

        JSON_ObjectAppendBool(resume_obj, "available", resume->available);
        JSON_ObjectAppendBool(resume_obj, "has_pistols", resume->has_pistols);
        JSON_ObjectAppendBool(resume_obj, "has_magnums", resume->has_magnums);
        JSON_ObjectAppendBool(resume_obj, "has_uzis", resume->has_uzis);
        JSON_ObjectAppendBool(resume_obj, "has_shotgun", resume->has_shotgun);
        JSON_ObjectAppendBool(resume_obj, "has_m16", resume->has_m16);
        JSON_ObjectAppendBool(resume_obj, "has_grenade", resume->has_grenade);
        JSON_ObjectAppendBool(resume_obj, "has_harpoon", resume->has_harpoon);

        JSON_ObjectAppendInt(resume_obj, "timer", resume->stats.timer);
        JSON_ObjectAppendInt(resume_obj, "ammo_hits", resume->stats.ammo_hits);
        JSON_ObjectAppendInt(resume_obj, "ammo_used", resume->stats.ammo_used);
        JSON_ObjectAppendInt(
            resume_obj, "distance_travelled", resume->stats.distance);
        JSON_ObjectAppendInt(resume_obj, "kills", resume->stats.kills);
        JSON_ObjectAppendInt(resume_obj, "secrets", resume->stats.secret_flags);
        JSON_ObjectAppendDouble(
            resume_obj, "medipacks_used", resume->stats.medipacks);
        JSON_ObjectAppendInt(
            resume_obj, "max_secrets", resume->stats.max_secret_count);
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
        resume->gun_type =
            JSON_ObjectGetInt(resume_obj, "gun_type", LGT_UNARMED);

        resume->available = JSON_ObjectGetBool(resume_obj, "available", 0);
        resume->has_pistols = JSON_ObjectGetBool(resume_obj, "has_pistols", 0);
        resume->has_magnums = JSON_ObjectGetBool(resume_obj, "has_magnums", 0);
        resume->has_uzis = JSON_ObjectGetBool(resume_obj, "has_uzis", 0);
        resume->has_shotgun = JSON_ObjectGetBool(resume_obj, "has_shotgun", 0);
        resume->has_m16 = JSON_ObjectGetBool(resume_obj, "has_m16", 0);
        resume->has_grenade = JSON_ObjectGetBool(resume_obj, "has_grenade", 0);
        resume->has_harpoon = JSON_ObjectGetBool(resume_obj, "has_harpoon", 0);

        resume->stats.timer = JSON_ObjectGetInt(resume_obj, "timer", 0);
        resume->stats.ammo_hits = JSON_ObjectGetInt(resume_obj, "ammo_hits", 0);
        resume->stats.ammo_used = JSON_ObjectGetInt(resume_obj, "ammo_used", 0);
        resume->stats.distance =
            JSON_ObjectGetInt(resume_obj, "distance_travelled", 0);
        resume->stats.kills = JSON_ObjectGetInt(resume_obj, "kills", 0);
        resume->stats.secret_flags =
            JSON_ObjectGetInt(resume_obj, "secrets", 0);
        resume->stats.medipacks =
            JSON_ObjectGetInt(resume_obj, "medipacks_used", 0);
        resume->stats.max_secret_count =
            JSON_ObjectGetInt(resume_obj, "max_secrets", 0);
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

static JSON_ARRAY *M_DumpItems(void)
{
    Savegame_ProcessItemsBeforeSave();

    JSON_ARRAY *const items_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        JSON_OBJECT *const item_obj = JSON_ObjectNew();
        const ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        JSON_ObjectAppendInt(item_obj, "obj_num", item->object_id);

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
            if (obj->intelligent) {
                JSON_ObjectAppendInt(
                    item_obj, "carried_item", item->carried_item);
            }
        }

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
            const LIFT_INFO *const data = (LIFT_INFO *)item->data;
            JSON_OBJECT *const data_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(data_obj, "start_height", data->start_height);
            JSON_ObjectAppendInt(data_obj, "wait_time", data->wait_time);
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

static bool M_LoadItems(JSON_ARRAY *const items_arr)
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

        const GAME_OBJECT_ID obj_id =
            JSON_ObjectGetInt(item_obj, "obj_num", -1);
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
            if (room_num != -1 && item->room_num != room_num) {
                Item_NewRoom(i, room_num);
            }
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
        }

        if (obj->save_hitpoints) {
            item->hit_points =
                JSON_ObjectGetInt(item_obj, "hitpoints", item->hit_points);
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

            if (obj->intelligent) {
                item->carried_item =
                    JSON_ObjectGetInt(item_obj, "carried_item", NO_ITEM);
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
                const JSON_OBJECT *const data_obj =
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

static bool M_IsValidItemObject(
    const GAME_OBJECT_ID saved_obj_id, const GAME_OBJECT_ID initial_obj_id)
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
    JSON_ObjectAppendInt(lara_obj, "dive_count", lara->dive_count);
    JSON_ObjectAppendInt(lara_obj, "death_count", lara->death_timer);
    JSON_ObjectAppendInt(lara_obj, "current_active", lara->current_active);
    JSON_ObjectAppendInt(lara_obj, "hit_effect_count", lara->hit_effect_count);
    JSON_ObjectAppendInt(lara_obj, "flare_age", lara->flare_age);
    JSON_ObjectAppendInt(lara_obj, "vehicle_item_number", lara->skidoo);
    JSON_ObjectAppendInt(lara_obj, "back_gun_obj_id", lara->back_gun);
    JSON_ObjectAppendInt(lara_obj, "flare_frame", lara->flare_frame);
    JSON_ObjectAppendInt(lara_obj, "mesh_effects", lara->mesh_effects);
    JSON_ObjectAppendInt(
        lara_obj, "water_surface_dist", lara->water_surface_dist);

    JSON_ObjectAppendBool(
        lara_obj, "flare_control_left", lara->flare_control_left);
    JSON_ObjectAppendBool(
        lara_obj, "flare_control_right", lara->flare_control_right);
    JSON_ObjectAppendBool(lara_obj, "extra_anim", lara->extra_anim);
    JSON_ObjectAppendBool(lara_obj, "look", lara->look);
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

    if (lara->weapon_item != NO_ITEM) {
        JSON_OBJECT *const weapon_obj = JSON_ObjectNew();
        const ITEM *const weapon_item = Item_Get(lara->weapon_item);
        JSON_ObjectAppendInt(weapon_obj, "obj_id", weapon_item->object_id);
        JSON_ObjectAppendInt(weapon_obj, "anim_num", weapon_item->anim_num);
        JSON_ObjectAppendInt(weapon_obj, "frame_num", weapon_item->frame_num);
        JSON_ObjectAppendInt(
            weapon_obj, "current_anim_state", weapon_item->current_anim_state);
        JSON_ObjectAppendInt(
            weapon_obj, "goal_anim_state", weapon_item->goal_anim_state);
        JSON_ObjectAppendObject(lara_obj, "weapon", weapon_obj);
    }

    return lara_obj;
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
    lara->dive_count =
        JSON_ObjectGetInt(lara_obj, "dive_count", lara->dive_count);
    lara->death_timer =
        JSON_ObjectGetInt(lara_obj, "death_count", lara->death_timer);
    lara->current_active =
        JSON_ObjectGetInt(lara_obj, "current_active", lara->current_active);
    lara->hit_effect_count =
        JSON_ObjectGetInt(lara_obj, "hit_effect_count", lara->hit_effect_count);
    lara->flare_age = JSON_ObjectGetInt(lara_obj, "flare_age", lara->flare_age);
    lara->skidoo =
        JSON_ObjectGetInt(lara_obj, "vehicle_item_number", lara->skidoo);
    lara->back_gun =
        JSON_ObjectGetInt(lara_obj, "back_gun_obj_id", lara->back_gun);
    lara->flare_frame =
        JSON_ObjectGetInt(lara_obj, "flare_frame", lara->flare_frame);
    lara->mesh_effects =
        JSON_ObjectGetInt(lara_obj, "mesh_effects", lara->mesh_effects);
    lara->water_surface_dist = JSON_ObjectGetInt(
        lara_obj, "water_surface_dist", lara->water_surface_dist);

    lara->flare_control_left = JSON_ObjectGetBool(
        lara_obj, "flare_control_left", lara->flare_control_left);
    lara->flare_control_right = JSON_ObjectGetBool(
        lara_obj, "flare_control_right", lara->flare_control_right);
    lara->extra_anim =
        JSON_ObjectGetBool(lara_obj, "extra_anim", lara->extra_anim);
    lara->look = JSON_ObjectGetBool(lara_obj, "look", lara->look);
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
        lara->weapon_item = Item_Create();
        ITEM *const weapon_item = Item_Get(lara->weapon_item);
        weapon_item->object_id =
            JSON_ObjectGetInt(weapon_obj, "obj_id", weapon_item->object_id);
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

static bool M_LoadArm(JSON_OBJECT *const arm_obj, LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    if (arm_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing arm info");
        return false;
    }

    arm->frame_num = JSON_ObjectGetInt(arm_obj, "frame_num", arm->frame_num);
    arm->lock = JSON_ObjectGetInt(arm_obj, "lock", arm->lock);
    arm->flash_gun = JSON_ObjectGetInt(arm_obj, "flash_gun", arm->flash_gun);
    LOAD_XYZ(arm_obj, "rot", arm->rot);

    return true;
}

static JSON_OBJECT *M_DumpAmmo(const AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    JSON_OBJECT *const ammo_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(ammo_obj, "ammo", ammo->ammo);
    return ammo_obj;
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

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
