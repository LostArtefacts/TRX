#include "game/savegame/savegame_bson.h"

#include "config.h"
#include "game/effects.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/room.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"
#include "json/bson_parse.h"
#include "json/bson_write.h"
#include "json/json_base.h"
#include "log.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <zconf.h>
#include <zlib.h>

#define SAVEGAME_BSON_MAGIC MKTAG('T', '1', 'M', 'B')

typedef struct SAVEGAME_BSON_HEADER {
    uint32_t magic;
    int16_t initial_version;
    uint16_t version;
    int32_t compressed_size;
    int32_t uncompressed_size;
} SAVEGAME_BSON_HEADER;

typedef struct SAVEGAME_BSON_FX_ORDER {
    int16_t count;
    int16_t id_map[NUM_EFFECTS];
} SAVEGAME_BSON_FX_ORDER;

static void SaveGame_BSON_SaveRaw(
    MYFILE *fp, struct json_value_s *root, int32_t version);
static struct json_value_s *Savegame_BSON_ParseFromBuffer(
    const char *buffer, size_t buffer_size, int32_t *version_out);
static struct json_value_s *Savegame_BSON_ParseFromFile(
    MYFILE *fp, int32_t *version_out);
static bool Savegame_BSON_LoadResumeInfo(
    struct json_array_s *levels_arr, RESUME_INFO *resume_info);
static bool Savegame_BSON_LoadDiscontinuedStartInfo(
    struct json_array_s *start_arr, GAME_INFO *game_info);
static bool Savegame_BSON_LoadDiscontinuedEndInfo(
    struct json_array_s *end_arr, GAME_INFO *game_info);
static bool Savegame_BSON_LoadMisc(
    struct json_object_s *misc_obj, GAME_INFO *game_info,
    uint16_t header_version);
static bool Savegame_BSON_LoadInventory(struct json_object_s *inv_obj);
static bool Savegame_BSON_LoadFlipmaps(struct json_object_s *flipmap_obj);
static bool Savegame_BSON_LoadCameras(struct json_array_s *cameras_arr);
static bool Savegame_BSON_LoadItems(
    struct json_array_s *items_arr, uint16_t header_version);
static bool SaveGame_BSON_LoadFx(struct json_array_s *fx_arr);
static bool Savegame_BSON_LoadArm(struct json_object_s *arm_obj, LARA_ARM *arm);
static bool Savegame_BSON_LoadAmmo(
    struct json_object_s *ammo_obj, AMMO_INFO *ammo);
static bool Savegame_BSON_LoadLOT(struct json_object_s *lot_obj, LOT_INFO *lot);
static bool Savegame_BSON_LoadLara(
    struct json_object_s *lara_obj, LARA_INFO *lara);
static bool SaveGame_BSON_LoadCurrentMusic(struct json_object_s *music_obj);
static bool SaveGame_BSON_LoadMusicTrackFlags(
    struct json_array_s *music_track_arr);
static struct json_array_s *Savegame_BSON_DumpResumeInfo(
    RESUME_INFO *game_info);
static struct json_object_s *Savegame_BSON_DumpMisc(GAME_INFO *game_info);
static struct json_object_s *Savegame_BSON_DumpInventory(void);
static struct json_object_s *Savegame_BSON_DumpFlipmaps(void);
static struct json_array_s *Savegame_BSON_DumpCameras(void);
static struct json_array_s *Savegame_BSON_DumpItems(void);
static struct json_array_s *SaveGame_BSON_DumpFx(void);
static struct json_object_s *Savegame_BSON_DumpArm(LARA_ARM *arm);
static struct json_object_s *Savegame_BSON_DumpAmmo(AMMO_INFO *ammo);
static struct json_object_s *Savegame_BSON_DumpLOT(LOT_INFO *lot);
static struct json_object_s *Savegame_BSON_DumpLara(LARA_INFO *lara);
static struct json_object_s *SaveGame_BSON_DumpCurrentMusic(void);
static struct json_array_s *SaveGame_BSON_DumpMusicTrackFlags(void);

static void SaveGame_BSON_GetFXOrder(SAVEGAME_BSON_FX_ORDER *order);
static bool Savegame_BSON_IsValidItemObject(
    int16_t saved_obj_num, int16_t current_obj_num);

static void SaveGame_BSON_SaveRaw(
    MYFILE *fp, struct json_value_s *root, int32_t version)
{
    size_t uncompressed_size;
    char *uncompressed = bson_write(root, &uncompressed_size);

    uLongf compressed_size = compressBound(uncompressed_size);
    char *compressed = Memory_Alloc(compressed_size);
    if (compress(
            (Bytef *)compressed, &compressed_size, (const Bytef *)uncompressed,
            (uLongf)uncompressed_size)
        != Z_OK) {
        Shell_ExitSystem("Failed to compress savegame data");
    }

    Memory_FreePointer(&uncompressed);

    SAVEGAME_BSON_HEADER header = {
        .magic = SAVEGAME_BSON_MAGIC,
        .initial_version = g_GameInfo.save_initial_version,
        .version = version,
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };

    File_Write(&header, sizeof(header), 1, fp);
    File_Write(compressed, sizeof(char), compressed_size, fp);

    Memory_FreePointer(&compressed);
}

static void SaveGame_BSON_GetFXOrder(SAVEGAME_BSON_FX_ORDER *order)
{
    order->count = 0;
    for (int i = 0; i < NUM_EFFECTS; i++) {
        order->id_map[i] = -1;
    }

    for (int16_t linknum = g_NextFxActive; linknum != NO_ITEM;
         linknum = g_Effects[linknum].next_active) {
        order->id_map[linknum] = order->count;
        order->count++;
    }
}

static bool Savegame_BSON_IsValidItemObject(
    int16_t saved_obj_num, int16_t initial_obj_num)
{
    if (saved_obj_num == initial_obj_num) {
        return true;
    }

    // clang-format off
    switch (saved_obj_num) {
        // used keyholes
        case O_PUZZLE_DONE1: return initial_obj_num == O_PUZZLE_HOLE1;
        case O_PUZZLE_DONE2: return initial_obj_num == O_PUZZLE_HOLE2;
        case O_PUZZLE_DONE3: return initial_obj_num == O_PUZZLE_HOLE3;
        case O_PUZZLE_DONE4: return initial_obj_num == O_PUZZLE_HOLE4;
        // pickups
        case O_GUN_AMMO_ITEM: return initial_obj_num == O_PISTOLS;
        case O_SG_AMMO_ITEM: return initial_obj_num == O_SHOTGUN_ITEM;
        case O_MAG_AMMO_ITEM: return initial_obj_num == O_MAGNUM_ITEM;
        case O_UZI_AMMO_ITEM: return initial_obj_num == O_UZI_ITEM;
        // dual-state animals
        case O_ALLIGATOR: return initial_obj_num == O_CROCODILE;
        case O_CROCODILE: return initial_obj_num == O_ALLIGATOR;
        case O_RAT: return initial_obj_num == O_VOLE;
        case O_VOLE: return initial_obj_num == O_RAT;
    }
    // clang-format on

    return false;
}

static struct json_value_s *Savegame_BSON_ParseFromBuffer(
    const char *buffer, size_t buffer_size, int32_t *version_out)
{
    SAVEGAME_BSON_HEADER *header = (SAVEGAME_BSON_HEADER *)buffer;
    if (header->magic != SAVEGAME_BSON_MAGIC) {
        LOG_ERROR("Invalid savegame magic");
        return NULL;
    }

    if (version_out) {
        *version_out = header->version;
    }

    const char *compressed = buffer + sizeof(SAVEGAME_BSON_HEADER);
    char *uncompressed = Memory_Alloc(header->uncompressed_size);

    uLongf uncompressed_size = header->uncompressed_size;
    int error_code = uncompress(
        (Bytef *)uncompressed, &uncompressed_size, (const Bytef *)compressed,
        (uLongf)header->compressed_size);
    if (error_code != Z_OK) {
        LOG_ERROR("Failed to decompress the data (error %d)", error_code);
        Memory_FreePointer(&uncompressed);
        return NULL;
    }

    struct json_value_s *root = bson_parse(uncompressed, uncompressed_size);
    Memory_FreePointer(&uncompressed);
    return root;
}

static struct json_value_s *Savegame_BSON_ParseFromFile(
    MYFILE *fp, int32_t *version_out)
{
    size_t buffer_size = File_Size(fp);
    char *buffer = Memory_Alloc(buffer_size);
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_Read(buffer, sizeof(char), buffer_size, fp);

    struct json_value_s *ret =
        Savegame_BSON_ParseFromBuffer(buffer, buffer_size, version_out);
    Memory_FreePointer(&buffer);
    return ret;
}

static bool Savegame_BSON_LoadResumeInfo(
    struct json_array_s *resume_arr, RESUME_INFO *resume_info)
{
    assert(resume_info);
    if (!resume_arr) {
        LOG_ERROR("Malformed save: invalid or missing resume array");
        return false;
    }
    if ((signed)resume_arr->length != g_GameFlow.level_count) {
        LOG_ERROR(
            "Malformed save: expected %d resume info elements, got %d",
            g_GameFlow.level_count, resume_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)resume_arr->length; i++) {
        struct json_object_s *resume_obj = json_array_get_object(resume_arr, i);
        if (!resume_obj) {
            LOG_ERROR("Malformed save: invalid resume info");
            return false;
        }
        RESUME_INFO *resume = &resume_info[i];
        resume->lara_hitpoints = json_object_get_int(
            resume_obj, "lara_hitpoints", g_Config.start_lara_hitpoints);
        resume->pistol_ammo = json_object_get_int(resume_obj, "pistol_ammo", 0);
        resume->magnum_ammo = json_object_get_int(resume_obj, "magnum_ammo", 0);
        resume->uzi_ammo = json_object_get_int(resume_obj, "uzi_ammo", 0);
        resume->shotgun_ammo =
            json_object_get_int(resume_obj, "shotgun_ammo", 0);
        resume->num_medis = json_object_get_int(resume_obj, "num_medis", 0);
        resume->num_big_medis =
            json_object_get_int(resume_obj, "num_big_medis", 0);
        resume->num_scions = json_object_get_int(resume_obj, "num_scions", 0);
        resume->gun_status = json_object_get_int(resume_obj, "gun_status", 0);
        resume->gun_type = json_object_get_int(resume_obj, "gun_type", 0);
        resume->flags.available =
            json_object_get_bool(resume_obj, "available", 0);
        resume->flags.got_pistols =
            json_object_get_bool(resume_obj, "got_pistols", 0);
        resume->flags.got_magnums =
            json_object_get_bool(resume_obj, "got_magnums", 0);
        resume->flags.got_uzis =
            json_object_get_bool(resume_obj, "got_uzis", 0);
        resume->flags.got_shotgun =
            json_object_get_bool(resume_obj, "got_shotgun", 0);
        resume->flags.costume = json_object_get_bool(resume_obj, "costume", 0);

        resume->stats.timer =
            json_object_get_int(resume_obj, "timer", resume->stats.timer);
        resume->stats.secret_flags = json_object_get_int(
            resume_obj, "secrets", resume->stats.secret_flags);
        resume->stats.kill_count =
            json_object_get_int(resume_obj, "kills", resume->stats.kill_count);
        resume->stats.pickup_count = json_object_get_int(
            resume_obj, "pickups", resume->stats.pickup_count);
        resume->stats.death_count = json_object_get_int(
            resume_obj, "deaths", resume->stats.death_count);
        resume->stats.max_secret_count = json_object_get_int(
            resume_obj, "max_secrets", resume->stats.max_secret_count);
        resume->stats.max_kill_count = json_object_get_int(
            resume_obj, "max_kills", resume->stats.max_kill_count);
        resume->stats.max_pickup_count = json_object_get_int(
            resume_obj, "max_pickups", resume->stats.max_pickup_count);
    }
    return true;
}

static bool Savegame_BSON_LoadDiscontinuedStartInfo(
    struct json_array_s *start_arr, GAME_INFO *game_info)
{
    // This function solely exists for backward compatibility with 2.6 and 2.7
    // saves.
    assert(game_info);
    assert(game_info->current);
    if (!start_arr) {
        LOG_ERROR(
            "Malformed save: invalid or missing discontinued start array");
        return false;
    }
    if ((signed)start_arr->length != g_GameFlow.level_count) {
        LOG_ERROR(
            "Malformed save: expected %d start info elements, got %d",
            g_GameFlow.level_count, start_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)start_arr->length; i++) {
        struct json_object_s *start_obj = json_array_get_object(start_arr, i);
        if (!start_obj) {
            LOG_ERROR("Malformed save: invalid discontinued start info");
            return false;
        }
        RESUME_INFO *start = &game_info->current[i];
        start->lara_hitpoints = json_object_get_int(
            start_obj, "lara_hitpoints", g_Config.start_lara_hitpoints);
        start->pistol_ammo = json_object_get_int(start_obj, "pistol_ammo", 0);
        start->magnum_ammo = json_object_get_int(start_obj, "magnum_ammo", 0);
        start->uzi_ammo = json_object_get_int(start_obj, "uzi_ammo", 0);
        start->shotgun_ammo = json_object_get_int(start_obj, "shotgun_ammo", 0);
        start->num_medis = json_object_get_int(start_obj, "num_medis", 0);
        start->num_big_medis =
            json_object_get_int(start_obj, "num_big_medis", 0);
        start->num_scions = json_object_get_int(start_obj, "num_scions", 0);
        start->gun_status = json_object_get_int(start_obj, "gun_status", 0);
        start->gun_type = json_object_get_int(start_obj, "gun_type", 0);
        start->flags.available =
            json_object_get_bool(start_obj, "available", 0);
        start->flags.got_pistols =
            json_object_get_bool(start_obj, "got_pistols", 0);
        start->flags.got_magnums =
            json_object_get_bool(start_obj, "got_magnums", 0);
        start->flags.got_uzis = json_object_get_bool(start_obj, "got_uzis", 0);
        start->flags.got_shotgun =
            json_object_get_bool(start_obj, "got_shotgun", 0);
        start->flags.costume = json_object_get_bool(start_obj, "costume", 0);
    }
    return true;
}

static bool Savegame_BSON_LoadDiscontinuedEndInfo(
    struct json_array_s *end_arr, GAME_INFO *game_info)
{
    // This function solely exists for backward compatibility with 2.6 and 2.7
    // saves.
    assert(game_info);
    assert(game_info->current);
    if (!end_arr) {
        LOG_ERROR("Malformed save: invalid or missing resume info array");
        return false;
    }
    if ((signed)end_arr->length != g_GameFlow.level_count) {
        LOG_ERROR(
            "Malformed save: expected %d resume info elements, got %d",
            g_GameFlow.level_count, end_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)end_arr->length; i++) {
        struct json_object_s *end_obj = json_array_get_object(end_arr, i);
        if (!end_obj) {
            LOG_ERROR("Malformed save: invalid resume info");
            return false;
        }
        GAME_STATS *end = &game_info->current[i].stats;
        end->timer = json_object_get_int(end_obj, "timer", end->timer);
        end->secret_flags =
            json_object_get_int(end_obj, "secrets", end->secret_flags);
        end->kill_count =
            json_object_get_int(end_obj, "kills", end->kill_count);
        end->pickup_count =
            json_object_get_int(end_obj, "pickups", end->pickup_count);
        end->death_count =
            json_object_get_int(end_obj, "deaths", end->death_count);
        end->max_secret_count =
            json_object_get_int(end_obj, "max_secrets", end->max_secret_count);
        end->max_kill_count =
            json_object_get_int(end_obj, "max_kills", end->max_kill_count);
        end->max_pickup_count =
            json_object_get_int(end_obj, "max_pickups", end->max_pickup_count);
    }
    game_info->death_counter_supported = true;
    return true;
}

static bool Savegame_BSON_LoadMisc(
    struct json_object_s *misc_obj, GAME_INFO *game_info,
    uint16_t header_version)
{
    assert(game_info);
    if (!misc_obj) {
        LOG_ERROR("Malformed save: invalid or missing misc info");
        return false;
    }
    game_info->bonus_flag = json_object_get_int(misc_obj, "bonus_flag", 0);
    if (header_version >= VERSION_3) {
        game_info->bonus_level_unlock =
            json_object_get_bool(misc_obj, "bonus_level_unlock", 0);
        LOG_DEBUG("Load bonus_level_unlock: %d", game_info->bonus_level_unlock);
    }
    return true;
}

static bool Savegame_BSON_LoadInventory(struct json_object_s *inv_obj)
{
    if (!inv_obj) {
        LOG_ERROR("Malformed save: invalid or missing inventory info");
        return false;
    }
    Lara_InitialiseInventory(g_CurrentLevel);
    Inv_AddItemNTimes(
        O_PICKUP_ITEM1, json_object_get_int(inv_obj, "pickup1", 0));
    Inv_AddItemNTimes(
        O_PICKUP_ITEM2, json_object_get_int(inv_obj, "pickup2", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM1, json_object_get_int(inv_obj, "puzzle1", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM2, json_object_get_int(inv_obj, "puzzle2", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM3, json_object_get_int(inv_obj, "puzzle3", 0));
    Inv_AddItemNTimes(
        O_PUZZLE_ITEM4, json_object_get_int(inv_obj, "puzzle4", 0));
    Inv_AddItemNTimes(O_KEY_ITEM1, json_object_get_int(inv_obj, "key1", 0));
    Inv_AddItemNTimes(O_KEY_ITEM2, json_object_get_int(inv_obj, "key2", 0));
    Inv_AddItemNTimes(O_KEY_ITEM3, json_object_get_int(inv_obj, "key3", 0));
    Inv_AddItemNTimes(O_KEY_ITEM4, json_object_get_int(inv_obj, "key4", 0));
    Inv_AddItemNTimes(
        O_LEADBAR_ITEM, json_object_get_int(inv_obj, "leadbar", 0));
    return true;
}

static bool Savegame_BSON_LoadFlipmaps(struct json_object_s *flipmap_obj)
{
    if (!flipmap_obj) {
        LOG_ERROR("Malformed save: invalid or missing flipmap info");
        return false;
    }

    if (json_object_get_bool(flipmap_obj, "status", false)) {
        Room_FlipMap();
    }

    g_FlipEffect = json_object_get_int(flipmap_obj, "effect", 0);
    g_FlipTimer = json_object_get_int(flipmap_obj, "timer", 0);

    struct json_array_s *flipmap_arr =
        json_object_get_array(flipmap_obj, "table");
    if (!flipmap_arr) {
        LOG_ERROR("Malformed save: invalid or missing flipmap table");
        return false;
    }
    if (flipmap_arr->length != MAX_FLIP_MAPS) {
        LOG_ERROR(
            "Malformed save: expected %d flipmap elements, got %d",
            MAX_FLIP_MAPS, flipmap_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)flipmap_arr->length; i++) {
        g_FlipMapTable[i] = json_array_get_int(flipmap_arr, i, 0) << 8;
    }

    return true;
}

static bool Savegame_BSON_LoadCameras(struct json_array_s *cameras_arr)
{
    if (!cameras_arr) {
        LOG_ERROR("Malformed save: invalid or missing cameras array");
        return false;
    }
    if ((signed)cameras_arr->length != g_NumberCameras) {
        LOG_ERROR(
            "Malformed save: expected %d cameras, got %d", g_NumberCameras,
            cameras_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)cameras_arr->length; i++) {
        g_Camera.fixed[i].flags = json_array_get_int(cameras_arr, i, 0);
    }
    return true;
}

static bool Savegame_BSON_LoadItems(
    struct json_array_s *items_arr, uint16_t header_version)
{
    if (!items_arr) {
        LOG_ERROR("Malformed save: invalid or missing items array");
        return false;
    }

    if ((signed)items_arr->length != g_LevelItemCount) {
        LOG_ERROR(
            "Malformed save: expected %d items, got %d", g_LevelItemCount,
            items_arr->length);
        return false;
    }

    for (int i = 0; i < (signed)items_arr->length; i++) {
        struct json_object_s *item_obj = json_array_get_object(items_arr, i);
        if (!item_obj) {
            LOG_ERROR("Malformed save: invalid item data");
            return false;
        }

        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        int obj_num = json_object_get_int(item_obj, "obj_num", -1);
        if (!Savegame_BSON_IsValidItemObject(obj_num, item->object_number)) {
            LOG_ERROR(
                "Malformed save: expected object %d, got %d",
                item->object_number, obj_num);
            return false;
        }

        if (obj->save_position) {
            item->pos.x = json_object_get_int(item_obj, "x", item->pos.x);
            item->pos.y = json_object_get_int(item_obj, "y", item->pos.y);
            item->pos.z = json_object_get_int(item_obj, "z", item->pos.z);
            item->pos.x_rot =
                json_object_get_int(item_obj, "x_rot", item->pos.x_rot);
            item->pos.y_rot =
                json_object_get_int(item_obj, "y_rot", item->pos.y_rot);
            item->pos.z_rot =
                json_object_get_int(item_obj, "z_rot", item->pos.z_rot);
            item->speed = json_object_get_int(item_obj, "speed", item->speed);
            item->fall_speed =
                json_object_get_int(item_obj, "fall_speed", item->fall_speed);

            int16_t room_num = json_object_get_int(item_obj, "room_num", -1);
            if (room_num != -1 && item->room_number != room_num) {
                Item_NewRoom(i, room_num);
            }
        }

        if (obj->save_anim) {
            item->current_anim_state = json_object_get_int(
                item_obj, "current_anim", item->current_anim_state);
            item->goal_anim_state = json_object_get_int(
                item_obj, "goal_anim", item->goal_anim_state);
            item->required_anim_state = json_object_get_int(
                item_obj, "required_anim", item->required_anim_state);
            item->anim_number =
                json_object_get_int(item_obj, "anim_num", item->anim_number);
            item->frame_number =
                json_object_get_int(item_obj, "frame_num", item->frame_number);
        }

        if (obj->save_hitpoints) {
            item->hit_points =
                json_object_get_int(item_obj, "hitpoints", item->hit_points);
        }

        if (obj->save_flags) {
            item->flags = json_object_get_int(item_obj, "flags", item->flags);
            item->timer = json_object_get_int(item_obj, "timer", item->timer);

            if (item->flags & IF_KILLED_ITEM) {
                Item_Kill(i);
                item->status = IS_DEACTIVATED;
            } else {
                if (json_object_get_bool(item_obj, "active", item->active)
                    && !item->active) {
                    Item_AddActive(i);
                }
                item->status =
                    json_object_get_int(item_obj, "status", item->status);
                item->gravity_status = json_object_get_bool(
                    item_obj, "gravity", item->gravity_status);
                item->collidable = json_object_get_bool(
                    item_obj, "collidable", item->collidable);
            }

            if (json_object_get_bool(
                    item_obj, "intelligent", obj->intelligent)) {
                LOT_EnableBaddieAI(i, 1);
                CREATURE_INFO *creature = item->data;
                if (creature) {
                    creature->head_rotation = json_object_get_int(
                        item_obj, "head_rot", creature->head_rotation);
                    creature->neck_rotation = json_object_get_int(
                        item_obj, "neck_rot", creature->neck_rotation);
                    creature->maximum_turn = json_object_get_int(
                        item_obj, "max_turn", creature->maximum_turn);
                    creature->flags = json_object_get_int(
                        item_obj, "creature_flags", creature->flags);
                    creature->mood = json_object_get_int(
                        item_obj, "creature_mood", creature->mood);
                }
            } else if (obj->intelligent) {
                item->data = NULL;
            }

            if (header_version >= VERSION_3
                && item->object_number == O_FLAME_EMITTER
                && g_Config.enable_enhanced_saves) {
                int32_t fx_num = json_object_get_int(item_obj, "fx_num", -1);
                if (fx_num != -1) {
                    item->data = (void *)(intptr_t)(fx_num + 1);
                }
            }
        }
    }

    return true;
}

static bool SaveGame_BSON_LoadFx(struct json_array_s *fx_arr)
{
    if (!g_Config.enable_enhanced_saves) {
        return true;
    }

    if (!fx_arr) {
        LOG_ERROR("Malformed save: invalid or missing fx array");
        return false;
    }

    if ((signed)fx_arr->length >= NUM_EFFECTS) {
        LOG_WARNING(
            "Malformed save: expected a max of %d fx, got %d. fx over the "
            "maximum will not be created.",
            NUM_EFFECTS - 1, fx_arr->length);
    }

    for (int i = 0; i < (signed)fx_arr->length; i++) {
        struct json_object_s *fx_obj = json_array_get_object(fx_arr, i);
        if (!fx_obj) {
            LOG_ERROR("Malformed save: invalid fx data");
            return false;
        }

        int32_t x = json_object_get_int(fx_obj, "x", 0);
        int32_t y = json_object_get_int(fx_obj, "y", 0);
        int32_t z = json_object_get_int(fx_obj, "z", 0);
        int16_t room_num = json_object_get_int(fx_obj, "room_number", 0);
        GAME_OBJECT_ID object_number =
            json_object_get_int(fx_obj, "object_number", 0);
        int16_t speed = json_object_get_int(fx_obj, "speed", 0);
        int16_t fall_speed = json_object_get_int(fx_obj, "fall_speed", 0);
        int16_t frame_number = json_object_get_int(fx_obj, "frame_number", 0);
        int16_t counter = json_object_get_int(fx_obj, "counter", 0);
        int16_t shade = json_object_get_int(fx_obj, "shade", 0);

        int16_t fx_num = Effect_Create(room_num);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = x;
            fx->pos.y = y;
            fx->pos.z = z;
            fx->object_number = object_number;
            fx->speed = speed;
            fx->fall_speed = fall_speed;
            fx->frame_number = frame_number;
            fx->counter = counter;
            fx->shade = shade;
        }
    }

    return true;
}

static bool Savegame_BSON_LoadArm(struct json_object_s *arm_obj, LARA_ARM *arm)
{
    assert(arm);
    if (!arm_obj) {
        LOG_ERROR("Malformed save: invalid or missing arm info");
        return false;
    }

    size_t idx = arm->frame_base - g_AnimFrames;
    idx = json_object_get_int(arm_obj, "frame_base", idx);
    arm->frame_base = &g_AnimFrames[idx];

    arm->frame_number =
        json_object_get_int(arm_obj, "frame_num", arm->frame_number);
    arm->lock = json_object_get_int(arm_obj, "lock", arm->lock);
    arm->x_rot = json_object_get_int(arm_obj, "x_rot", arm->x_rot);
    arm->y_rot = json_object_get_int(arm_obj, "y_rot", arm->y_rot);
    arm->z_rot = json_object_get_int(arm_obj, "z_rot", arm->z_rot);
    arm->flash_gun = json_object_get_int(arm_obj, "flash_gun", arm->flash_gun);
    return true;
}

static bool Savegame_BSON_LoadAmmo(
    struct json_object_s *ammo_obj, AMMO_INFO *ammo)
{
    assert(ammo);
    if (!ammo_obj) {
        LOG_ERROR("Malformed save: invalid or missing ammo info");
        return false;
    }

    ammo->ammo = json_object_get_int(ammo_obj, "ammo", ammo->ammo);
    ammo->hit = json_object_get_int(ammo_obj, "hit", ammo->hit);
    ammo->miss = json_object_get_int(ammo_obj, "miss", ammo->miss);
    return true;
}

static bool Savegame_BSON_LoadLOT(struct json_object_s *lot_obj, LOT_INFO *lot)
{
    assert(lot);
    if (!lot_obj) {
        LOG_ERROR("Malformed save: invalid or missing LOT info");
        return false;
    }

    lot->head = json_object_get_int(lot_obj, "head", lot->head);
    lot->tail = json_object_get_int(lot_obj, "tail", lot->tail);
    lot->search_number =
        json_object_get_int(lot_obj, "search_num", lot->search_number);
    lot->block_mask =
        json_object_get_int(lot_obj, "block_mask", lot->block_mask);
    lot->step = json_object_get_int(lot_obj, "step", lot->step);
    lot->drop = json_object_get_int(lot_obj, "drop", lot->drop);
    lot->fly = json_object_get_int(lot_obj, "fly", lot->fly);
    lot->zone_count =
        json_object_get_int(lot_obj, "zone_count", lot->zone_count);
    lot->target_box =
        json_object_get_int(lot_obj, "target_box", lot->target_box);
    lot->required_box =
        json_object_get_int(lot_obj, "required_box", lot->required_box);
    lot->target.x = json_object_get_int(lot_obj, "x", lot->target.x);
    lot->target.y = json_object_get_int(lot_obj, "y", lot->target.y);
    lot->target.z = json_object_get_int(lot_obj, "z", lot->target.z);
    return true;
}

static bool Savegame_BSON_LoadLara(
    struct json_object_s *lara_obj, LARA_INFO *lara)
{
    assert(lara);
    if (!lara_obj) {
        LOG_ERROR("Malformed save: invalid or missing Lara info");
        return false;
    }

    lara->item_number =
        json_object_get_int(lara_obj, "item_number", lara->item_number);
    lara->gun_status =
        json_object_get_int(lara_obj, "gun_status", lara->gun_status);
    lara->gun_type = json_object_get_int(lara_obj, "gun_type", lara->gun_type);
    lara->request_gun_type = json_object_get_int(
        lara_obj, "request_gun_type", lara->request_gun_type);
    lara->calc_fall_speed =
        json_object_get_int(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    lara->water_status =
        json_object_get_int(lara_obj, "water_status", lara->water_status);
    lara->pose_count =
        json_object_get_int(lara_obj, "pose_count", lara->pose_count);
    lara->hit_frame =
        json_object_get_int(lara_obj, "hit_frame", lara->hit_frame);
    lara->hit_direction =
        json_object_get_int(lara_obj, "hit_direction", lara->hit_direction);
    lara->air = json_object_get_int(lara_obj, "air", lara->air);
    lara->dive_timer =
        json_object_get_int(lara_obj, "dive_count", lara->dive_timer);
    lara->death_timer =
        json_object_get_int(lara_obj, "death_count", lara->death_timer);
    lara->current_active =
        json_object_get_int(lara_obj, "current_active", lara->current_active);

    lara->spaz_effect_count = json_object_get_int(
        lara_obj, "spaz_effect_count", lara->spaz_effect_count);
    int spaz_effect = json_object_get_int(lara_obj, "spaz_effect", 0);
    lara->spaz_effect = spaz_effect && g_Config.enable_enhanced_saves
        ? &g_Effects[spaz_effect]
        : NULL;

    lara->mesh_effects =
        json_object_get_int(lara_obj, "mesh_effects", lara->mesh_effects);

    struct json_array_s *lara_meshes_arr =
        json_object_get_array(lara_obj, "meshes");
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
    for (int i = 0; i < (signed)lara_meshes_arr->length; i++) {
        size_t idx = lara->mesh_ptrs[i] - g_MeshBase;
        idx = json_array_get_int(lara_meshes_arr, i, idx);
        lara->mesh_ptrs[i] = &g_MeshBase[idx];
    }

    lara->target = NULL;

    lara->target_angles[0] =
        json_object_get_int(lara_obj, "target_angle1", lara->target_angles[0]);
    lara->target_angles[1] =
        json_object_get_int(lara_obj, "target_angle2", lara->target_angles[1]);
    lara->turn_rate =
        json_object_get_int(lara_obj, "turn_rate", lara->turn_rate);
    lara->move_angle =
        json_object_get_int(lara_obj, "move_angle", lara->move_angle);
    lara->head_y_rot =
        json_object_get_int(lara_obj, "head_y_rot", lara->head_y_rot);
    lara->head_x_rot =
        json_object_get_int(lara_obj, "head_x_rot", lara->head_x_rot);
    lara->head_z_rot =
        json_object_get_int(lara_obj, "head_z_rot", lara->head_z_rot);
    lara->torso_y_rot =
        json_object_get_int(lara_obj, "torso_y_rot", lara->torso_y_rot);
    lara->torso_x_rot =
        json_object_get_int(lara_obj, "torso_x_rot", lara->torso_x_rot);
    lara->torso_z_rot =
        json_object_get_int(lara_obj, "torso_z_rot", lara->torso_z_rot);

    if (!Savegame_BSON_LoadArm(
            json_object_get_object(lara_obj, "left_arm"), &lara->left_arm)) {
        return false;
    }

    if (!Savegame_BSON_LoadArm(
            json_object_get_object(lara_obj, "right_arm"), &lara->right_arm)) {
        return false;
    }

    if (!Savegame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "pistols"), &lara->pistols)) {
        return false;
    }

    if (!Savegame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "magnums"), &lara->magnums)) {
        return false;
    }

    if (!Savegame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "uzis"), &lara->uzis)) {
        return false;
    }

    if (!Savegame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "shotgun"), &lara->shotgun)) {
        return false;
    }

    if (!Savegame_BSON_LoadLOT(
            json_object_get_object(lara_obj, "lot"), &lara->LOT)) {
        return false;
    }

    return true;
}

static bool SaveGame_BSON_LoadCurrentMusic(struct json_object_s *music_obj)
{
    if (!g_Config.load_current_music) {
        return true;
    }

    if (!music_obj) {
        LOG_WARNING("Malformed save: invalid or missing current music");
        return true;
    }

    int16_t current_track = json_object_get_int(music_obj, "current_track", -1);
    int64_t timestamp = json_object_get_int64(music_obj, "timestamp", -1);
    if (current_track != MX_INACTIVE) {
        Music_Play(current_track);
        if (!Music_SeekTimestamp(timestamp)) {
            LOG_WARNING(
                "Could not load current track %d at timestamp %" PRId64 ".",
                current_track, timestamp);
        }
    }
    return true;
}

static bool SaveGame_BSON_LoadMusicTrackFlags(
    struct json_array_s *music_track_arr)
{
    if (!music_track_arr) {
        LOG_WARNING("Malformed save: invalid or missing music track array");
        return true;
    }

    if ((signed)music_track_arr->length != MAX_CD_TRACKS) {
        LOG_WARNING(
            "Malformed save: expected %d music track flags, got %d",
            MAX_CD_TRACKS, music_track_arr->length);
        return true;
    }

    for (int i = 0; i < (signed)music_track_arr->length; i++) {
        g_MusicTrackFlags[i] = json_array_get_int(music_track_arr, i, 0);
    }

    return true;
}

static struct json_array_s *Savegame_BSON_DumpResumeInfo(
    RESUME_INFO *resume_info)
{
    struct json_array_s *resume_arr = json_array_new();
    assert(resume_info);
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        RESUME_INFO *resume = &resume_info[i];
        struct json_object_s *resume_obj = json_object_new();
        json_object_append_int(
            resume_obj, "lara_hitpoints", resume->lara_hitpoints);
        json_object_append_int(resume_obj, "pistol_ammo", resume->pistol_ammo);
        json_object_append_int(resume_obj, "magnum_ammo", resume->magnum_ammo);
        json_object_append_int(resume_obj, "uzi_ammo", resume->uzi_ammo);
        json_object_append_int(
            resume_obj, "shotgun_ammo", resume->shotgun_ammo);
        json_object_append_int(resume_obj, "num_medis", resume->num_medis);
        json_object_append_int(
            resume_obj, "num_big_medis", resume->num_big_medis);
        json_object_append_int(resume_obj, "num_scions", resume->num_scions);
        json_object_append_int(resume_obj, "gun_status", resume->gun_status);
        json_object_append_int(resume_obj, "gun_type", resume->gun_type);
        json_object_append_bool(
            resume_obj, "available", resume->flags.available);
        json_object_append_bool(
            resume_obj, "got_pistols", resume->flags.got_pistols);
        json_object_append_bool(
            resume_obj, "got_magnums", resume->flags.got_magnums);
        json_object_append_bool(resume_obj, "got_uzis", resume->flags.got_uzis);
        json_object_append_bool(
            resume_obj, "got_shotgun", resume->flags.got_shotgun);
        json_object_append_bool(resume_obj, "costume", resume->flags.costume);
        json_object_append_int(resume_obj, "timer", resume->stats.timer);
        json_object_append_int(resume_obj, "kills", resume->stats.kill_count);
        json_object_append_int(
            resume_obj, "secrets", resume->stats.secret_flags);
        json_object_append_int(
            resume_obj, "pickups", resume->stats.pickup_count);
        json_object_append_int(resume_obj, "deaths", resume->stats.death_count);
        json_object_append_int(
            resume_obj, "max_kills", resume->stats.max_kill_count);
        json_object_append_int(
            resume_obj, "max_secrets", resume->stats.max_secret_count);
        json_object_append_int(
            resume_obj, "max_pickups", resume->stats.max_pickup_count);
        json_array_append_object(resume_arr, resume_obj);
    }
    return resume_arr;
}

static struct json_object_s *Savegame_BSON_DumpMisc(GAME_INFO *game_info)
{
    assert(game_info);
    struct json_object_s *misc_obj = json_object_new();
    json_object_append_int(misc_obj, "bonus_flag", game_info->bonus_flag);
    json_object_append_bool(
        misc_obj, "bonus_level_unlock", game_info->bonus_level_unlock);
    return misc_obj;
}

static struct json_object_s *Savegame_BSON_DumpInventory(void)
{
    struct json_object_s *inv_obj = json_object_new();
    json_object_append_int(inv_obj, "pickup1", Inv_RequestItem(O_PICKUP_ITEM1));
    json_object_append_int(inv_obj, "pickup2", Inv_RequestItem(O_PICKUP_ITEM2));
    json_object_append_int(inv_obj, "puzzle1", Inv_RequestItem(O_PUZZLE_ITEM1));
    json_object_append_int(inv_obj, "puzzle2", Inv_RequestItem(O_PUZZLE_ITEM2));
    json_object_append_int(inv_obj, "puzzle3", Inv_RequestItem(O_PUZZLE_ITEM3));
    json_object_append_int(inv_obj, "puzzle4", Inv_RequestItem(O_PUZZLE_ITEM4));
    json_object_append_int(inv_obj, "key1", Inv_RequestItem(O_KEY_ITEM1));
    json_object_append_int(inv_obj, "key2", Inv_RequestItem(O_KEY_ITEM2));
    json_object_append_int(inv_obj, "key3", Inv_RequestItem(O_KEY_ITEM3));
    json_object_append_int(inv_obj, "key4", Inv_RequestItem(O_KEY_ITEM4));
    json_object_append_int(inv_obj, "leadbar", Inv_RequestItem(O_LEADBAR_ITEM));
    return inv_obj;
}

static struct json_object_s *Savegame_BSON_DumpFlipmaps(void)
{
    struct json_object_s *flipmap_obj = json_object_new();
    json_object_append_bool(flipmap_obj, "status", g_FlipStatus);
    json_object_append_int(flipmap_obj, "effect", g_FlipEffect);
    json_object_append_int(flipmap_obj, "timer", g_FlipTimer);
    struct json_array_s *flipmap_arr = json_array_new();
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        json_array_append_int(flipmap_arr, g_FlipMapTable[i] >> 8);
    }
    json_object_append_array(flipmap_obj, "table", flipmap_arr);
    return flipmap_obj;
}

static struct json_array_s *Savegame_BSON_DumpCameras(void)
{
    struct json_array_s *cameras_arr = json_array_new();
    for (int i = 0; i < g_NumberCameras; i++) {
        json_array_append_int(cameras_arr, g_Camera.fixed[i].flags);
    }
    return cameras_arr;
}

static struct json_array_s *Savegame_BSON_DumpItems(void)
{
    SAVEGAME_BSON_FX_ORDER fx_order;
    SaveGame_BSON_GetFXOrder(&fx_order);

    struct json_array_s *items_arr = json_array_new();
    for (int i = 0; i < g_LevelItemCount; i++) {
        struct json_object_s *item_obj = json_object_new();
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        json_object_append_int(item_obj, "obj_num", item->object_number);

        if (obj->save_position) {
            json_object_append_int(item_obj, "x", item->pos.x);
            json_object_append_int(item_obj, "y", item->pos.y);
            json_object_append_int(item_obj, "z", item->pos.z);
            json_object_append_int(item_obj, "x_rot", item->pos.x_rot);
            json_object_append_int(item_obj, "y_rot", item->pos.y_rot);
            json_object_append_int(item_obj, "z_rot", item->pos.z_rot);
            json_object_append_int(item_obj, "room_num", item->room_number);
            json_object_append_int(item_obj, "speed", item->speed);
            json_object_append_int(item_obj, "fall_speed", item->fall_speed);
        }

        if (obj->save_anim) {
            json_object_append_int(
                item_obj, "current_anim", item->current_anim_state);
            json_object_append_int(
                item_obj, "goal_anim", item->goal_anim_state);
            json_object_append_int(
                item_obj, "required_anim", item->required_anim_state);
            json_object_append_int(item_obj, "anim_num", item->anim_number);
            json_object_append_int(item_obj, "frame_num", item->frame_number);
        }

        if (obj->save_hitpoints) {
            json_object_append_int(item_obj, "hitpoints", item->hit_points);
        }

        if (obj->save_flags) {
            json_object_append_int(item_obj, "flags", item->flags);
            json_object_append_int(item_obj, "status", item->status);
            json_object_append_bool(item_obj, "active", item->active);
            json_object_append_bool(item_obj, "gravity", item->gravity_status);
            json_object_append_bool(item_obj, "collidable", item->collidable);
            json_object_append_bool(
                item_obj, "intelligent", obj->intelligent && item->data);
            json_object_append_int(item_obj, "timer", item->timer);
            if (obj->intelligent && item->data) {
                CREATURE_INFO *creature = item->data;
                json_object_append_int(
                    item_obj, "head_rot", creature->head_rotation);
                json_object_append_int(
                    item_obj, "neck_rot", creature->neck_rotation);
                json_object_append_int(
                    item_obj, "max_turn", creature->maximum_turn);
                json_object_append_int(
                    item_obj, "creature_flags", creature->flags);
                json_object_append_int(
                    item_obj, "creature_mood", creature->mood);
            }

            if (item->object_number == O_FLAME_EMITTER && item->data) {
                int32_t fx_num = (int32_t)(intptr_t)item->data - 1;
                fx_num = fx_order.id_map[fx_num];
                json_object_append_int(item_obj, "fx_num", fx_num);
            }
        }

        json_array_append_object(items_arr, item_obj);
    }
    return items_arr;
}

static struct json_array_s *SaveGame_BSON_DumpFx(void)
{
    struct json_array_s *fx_arr = json_array_new();

    SAVEGAME_BSON_FX_ORDER fx_order;
    SaveGame_BSON_GetFXOrder(&fx_order);

    for (int16_t linknum = g_NextFxActive; linknum != NO_ITEM;
         linknum = g_Effects[linknum].next_active) {
        struct json_object_s *fx_obj = json_object_new();
        FX_INFO *fx = &g_Effects[linknum];
        json_object_append_int(fx_obj, "x", fx->pos.x);
        json_object_append_int(fx_obj, "y", fx->pos.y);
        json_object_append_int(fx_obj, "z", fx->pos.z);
        json_object_append_int(fx_obj, "room_number", fx->room_number);
        json_object_append_int(fx_obj, "object_number", fx->object_number);
        json_object_append_int(fx_obj, "speed", fx->speed);
        json_object_append_int(fx_obj, "fall_speed", fx->fall_speed);
        json_object_append_int(fx_obj, "frame_number", fx->frame_number);
        json_object_append_int(fx_obj, "counter", fx->counter);
        json_object_append_int(fx_obj, "shade", fx->shade);
        json_array_append_object(fx_arr, fx_obj);
    }

    return fx_arr;
}

static struct json_object_s *Savegame_BSON_DumpArm(LARA_ARM *arm)
{
    assert(arm);
    struct json_object_s *arm_obj = json_object_new();
    json_object_append_int(
        arm_obj, "frame_base", arm->frame_base - g_AnimFrames);
    json_object_append_int(arm_obj, "frame_num", arm->frame_number);
    json_object_append_int(arm_obj, "lock", arm->lock);
    json_object_append_int(arm_obj, "x_rot", arm->x_rot);
    json_object_append_int(arm_obj, "y_rot", arm->y_rot);
    json_object_append_int(arm_obj, "z_rot", arm->z_rot);
    json_object_append_int(arm_obj, "flash_gun", arm->flash_gun);
    return arm_obj;
}

static struct json_object_s *Savegame_BSON_DumpAmmo(AMMO_INFO *ammo)
{
    assert(ammo);
    struct json_object_s *ammo_obj = json_object_new();
    json_object_append_int(ammo_obj, "ammo", ammo->ammo);
    json_object_append_int(ammo_obj, "hit", ammo->hit);
    json_object_append_int(ammo_obj, "miss", ammo->miss);
    return ammo_obj;
}

static struct json_object_s *Savegame_BSON_DumpLOT(LOT_INFO *lot)
{
    assert(lot);
    struct json_object_s *lot_obj = json_object_new();
    // json_object_append_int(lot_obj, "node", lot->node);
    json_object_append_int(lot_obj, "head", lot->head);
    json_object_append_int(lot_obj, "tail", lot->tail);
    json_object_append_int(lot_obj, "search_num", lot->search_number);
    json_object_append_int(lot_obj, "block_mask", lot->block_mask);
    json_object_append_int(lot_obj, "step", lot->step);
    json_object_append_int(lot_obj, "drop", lot->drop);
    json_object_append_int(lot_obj, "fly", lot->fly);
    json_object_append_int(lot_obj, "zone_count", lot->zone_count);
    json_object_append_int(lot_obj, "target_box", lot->target_box);
    json_object_append_int(lot_obj, "required_box", lot->required_box);
    json_object_append_int(lot_obj, "x", lot->target.x);
    json_object_append_int(lot_obj, "y", lot->target.y);
    json_object_append_int(lot_obj, "z", lot->target.z);
    return lot_obj;
}

static struct json_object_s *Savegame_BSON_DumpLara(LARA_INFO *lara)
{
    assert(lara);
    struct json_object_s *lara_obj = json_object_new();
    json_object_append_int(lara_obj, "item_number", lara->item_number);
    json_object_append_int(lara_obj, "gun_status", lara->gun_status);
    json_object_append_int(lara_obj, "gun_type", lara->gun_type);
    json_object_append_int(
        lara_obj, "request_gun_type", lara->request_gun_type);
    json_object_append_int(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    json_object_append_int(lara_obj, "water_status", lara->water_status);
    json_object_append_int(lara_obj, "pose_count", lara->pose_count);
    json_object_append_int(lara_obj, "hit_frame", lara->hit_frame);
    json_object_append_int(lara_obj, "hit_direction", lara->hit_direction);
    json_object_append_int(lara_obj, "air", lara->air);
    json_object_append_int(lara_obj, "dive_count", lara->dive_timer);
    json_object_append_int(lara_obj, "death_count", lara->death_timer);
    json_object_append_int(lara_obj, "current_active", lara->current_active);

    json_object_append_int(
        lara_obj, "spaz_effect_count", lara->spaz_effect_count);
    json_object_append_int(
        lara_obj, "spaz_effect",
        lara->spaz_effect ? lara->spaz_effect - g_Effects : 0);

    json_object_append_int(lara_obj, "mesh_effects", lara->mesh_effects);
    struct json_array_s *lara_meshes_arr = json_array_new();
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        json_array_append_int(lara_meshes_arr, lara->mesh_ptrs[i] - g_MeshBase);
    }
    json_object_append_array(lara_obj, "meshes", lara_meshes_arr);

    json_object_append_int(lara_obj, "target_angle1", lara->target_angles[0]);
    json_object_append_int(lara_obj, "target_angle2", lara->target_angles[1]);
    json_object_append_int(lara_obj, "turn_rate", lara->turn_rate);
    json_object_append_int(lara_obj, "move_angle", lara->move_angle);
    json_object_append_int(lara_obj, "head_y_rot", lara->head_y_rot);
    json_object_append_int(lara_obj, "head_x_rot", lara->head_x_rot);
    json_object_append_int(lara_obj, "head_z_rot", lara->head_z_rot);
    json_object_append_int(lara_obj, "torso_y_rot", lara->torso_y_rot);
    json_object_append_int(lara_obj, "torso_x_rot", lara->torso_x_rot);
    json_object_append_int(lara_obj, "torso_z_rot", lara->torso_z_rot);

    json_object_append_object(
        lara_obj, "left_arm", Savegame_BSON_DumpArm(&lara->left_arm));
    json_object_append_object(
        lara_obj, "right_arm", Savegame_BSON_DumpArm(&lara->right_arm));
    json_object_append_object(
        lara_obj, "pistols", Savegame_BSON_DumpAmmo(&lara->pistols));
    json_object_append_object(
        lara_obj, "magnums", Savegame_BSON_DumpAmmo(&lara->magnums));
    json_object_append_object(
        lara_obj, "uzis", Savegame_BSON_DumpAmmo(&lara->uzis));
    json_object_append_object(
        lara_obj, "shotgun", Savegame_BSON_DumpAmmo(&lara->shotgun));
    json_object_append_object(
        lara_obj, "lot", Savegame_BSON_DumpLOT(&lara->LOT));

    return lara_obj;
}

static struct json_object_s *SaveGame_BSON_DumpCurrentMusic(void)
{
    struct json_object_s *current_music_obj = json_object_new();
    json_object_append_int(
        current_music_obj, "current_track", Music_GetCurrentTrack());
    json_object_append_int64(
        current_music_obj, "timestamp", Music_GetTimestamp());

    return current_music_obj;
}

static struct json_array_s *SaveGame_BSON_DumpMusicTrackFlags(void)
{
    struct json_array_s *music_track_arr = json_array_new();
    for (int i = 0; i < MAX_CD_TRACKS; i++) {
        json_array_append_int(music_track_arr, g_MusicTrackFlags[i]);
    }
    return music_track_arr;
}

char *Savegame_BSON_GetSaveFileName(int32_t slot)
{
    size_t out_size = snprintf(NULL, 0, g_GameFlow.savegame_fmt_bson, slot) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, g_GameFlow.savegame_fmt_bson, slot);
    return out;
}

bool Savegame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info)
{
    bool ret = false;
    struct json_value_s *root = Savegame_BSON_ParseFromFile(fp, NULL);
    struct json_object_s *root_obj = json_value_as_object(root);
    if (root_obj) {
        info->counter = json_object_get_int(root_obj, "save_counter", -1);
        info->level_num = json_object_get_int(root_obj, "level_num", -1);
        const char *level_title =
            json_object_get_string(root_obj, "level_title", NULL);
        if (level_title) {
            info->level_title = Memory_DupStr(level_title);
        }
        ret = info->level_num != -1;
    }
    json_value_free(root);

    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_Read(&header, sizeof(SAVEGAME_BSON_HEADER), 1, fp);
    info->initial_version = header.initial_version;
    info->features.restart = header.initial_version >= VERSION_LEGACY;
    info->features.select_level = header.initial_version >= VERSION_1;

    return ret;
}

bool Savegame_BSON_LoadFromFile(MYFILE *fp, GAME_INFO *game_info)
{
    assert(game_info);

    bool ret = false;

    // Read savegame version
    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_Read(&header, sizeof(SAVEGAME_BSON_HEADER), 1, fp);
    File_Seek(fp, 0, FILE_SEEK_SET);

    struct json_value_s *root = Savegame_BSON_ParseFromFile(fp, NULL);
    struct json_object_s *root_obj = json_value_as_object(root);
    if (!root_obj) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    g_CurrentLevel = json_object_get_int(root_obj, "level_num", -1);
    if (g_CurrentLevel < 0 || g_CurrentLevel >= g_GameFlow.level_count) {
        LOG_ERROR("Malformed save: invalid or missing level number");
        goto cleanup;
    }

    if (!Savegame_BSON_LoadResumeInfo(
            json_object_get_array(root_obj, "current_info"),
            game_info->current)) {
        LOG_WARNING("Failed to load RESUME_INFO current properly. Checking if "
                    "save is legacy.");
        // Check for 2.6 and 2.7 legacy start and end info.
        if (!Savegame_BSON_LoadDiscontinuedStartInfo(
                json_object_get_array(root_obj, "start_info"), game_info)) {
            goto cleanup;
        }
        if (!Savegame_BSON_LoadDiscontinuedEndInfo(
                json_object_get_array(root_obj, "end_info"), game_info)) {
            goto cleanup;
        }
    }

    if (!Savegame_BSON_LoadMisc(
            json_object_get_object(root_obj, "misc"), game_info,
            header.version)) {
        goto cleanup;
    }

    if (!Savegame_BSON_LoadInventory(
            json_object_get_object(root_obj, "inventory"))) {
        goto cleanup;
    }

    if (!Savegame_BSON_LoadFlipmaps(
            json_object_get_object(root_obj, "flipmap"))) {
        goto cleanup;
    }

    if (!Savegame_BSON_LoadCameras(
            json_object_get_array(root_obj, "cameras"))) {
        goto cleanup;
    }

    Savegame_PreprocessItems();

    if (!Savegame_BSON_LoadItems(
            json_object_get_array(root_obj, "items"), header.version)) {
        goto cleanup;
    }

    if (header.version >= VERSION_3) {
        if (!SaveGame_BSON_LoadFx(json_object_get_array(root_obj, "fx"))) {
            goto cleanup;
        }
    }

    if (!Savegame_BSON_LoadLara(
            json_object_get_object(root_obj, "lara"), &g_Lara)) {
        goto cleanup;
    }

    if (header.version >= VERSION_3) {
        if (!SaveGame_BSON_LoadCurrentMusic(
                json_object_get_object(root_obj, "music"))) {
            goto cleanup;
        }

        if (!SaveGame_BSON_LoadMusicTrackFlags(
                json_object_get_array(root_obj, "music_track_flags"))) {
            goto cleanup;
        }
    }

    ret = true;

cleanup:
    json_value_free(root);
    return ret;
}

bool Savegame_BSON_LoadOnlyResumeInfo(MYFILE *fp, GAME_INFO *game_info)
{
    assert(game_info);

    bool ret = false;
    struct json_value_s *root = Savegame_BSON_ParseFromFile(fp, NULL);
    struct json_object_s *root_obj = json_value_as_object(root);
    if (!root_obj) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!Savegame_BSON_LoadResumeInfo(
            json_object_get_array(root_obj, "current_info"),
            game_info->current)) {
        LOG_WARNING("Failed to load RESUME_INFO current properly. Checking if "
                    "save is legacy.");
        // Check for 2.6 and 2.7 legacy start and end info.
        if (!Savegame_BSON_LoadDiscontinuedStartInfo(
                json_object_get_array(root_obj, "start_info"), game_info)) {
            goto cleanup;
        }
        if (!Savegame_BSON_LoadDiscontinuedEndInfo(
                json_object_get_array(root_obj, "end_info"), game_info)) {
            goto cleanup;
        }
    }

    ret = true;

cleanup:
    json_value_free(root);
    return ret;
}

void Savegame_BSON_SaveToFile(MYFILE *fp, GAME_INFO *game_info)
{
    assert(game_info);

    struct json_object_s *root_obj = json_object_new();

    json_object_append_string(
        root_obj, "level_title", g_GameFlow.levels[g_CurrentLevel].level_title);
    json_object_append_int(root_obj, "save_counter", g_SaveCounter);
    json_object_append_int(root_obj, "level_num", g_CurrentLevel);

    json_object_append_object(
        root_obj, "misc", Savegame_BSON_DumpMisc(game_info));
    json_object_append_array(
        root_obj, "current_info",
        Savegame_BSON_DumpResumeInfo(game_info->current));
    json_object_append_object(
        root_obj, "inventory", Savegame_BSON_DumpInventory());
    json_object_append_object(
        root_obj, "flipmap", Savegame_BSON_DumpFlipmaps());
    json_object_append_array(root_obj, "cameras", Savegame_BSON_DumpCameras());
    json_object_append_array(root_obj, "items", Savegame_BSON_DumpItems());
    json_object_append_array(root_obj, "fx", SaveGame_BSON_DumpFx());
    json_object_append_object(
        root_obj, "lara", Savegame_BSON_DumpLara(&g_Lara));
    json_object_append_object(
        root_obj, "music", SaveGame_BSON_DumpCurrentMusic());
    json_object_append_array(
        root_obj, "music_track_flags", SaveGame_BSON_DumpMusicTrackFlags());

    struct json_value_s *root = json_value_from_object(root_obj);
    SaveGame_BSON_SaveRaw(fp, root, SAVEGAME_CURRENT_VERSION);
    json_value_free(root);
}

bool Savegame_BSON_UpdateDeathCounters(MYFILE *fp, GAME_INFO *game_info)
{
    bool ret = false;
    int32_t version;
    struct json_value_s *root = Savegame_BSON_ParseFromFile(fp, &version);
    struct json_object_s *root_obj = json_value_as_object(root);
    if (!root_obj) {
        LOG_ERROR("Cannot find the root object");
        goto cleanup;
    }

    struct json_array_s *current_arr =
        json_object_get_array(root_obj, "current_info");
    if (!current_arr) {
        current_arr = json_object_get_array(root_obj, "end_info");
        if (!current_arr) {
            LOG_ERROR("Malformed save: invalid or missing current info array");
            goto cleanup;
        }
    }
    if ((signed)current_arr->length != g_GameFlow.level_count) {
        LOG_ERROR(
            "Malformed save: expected %d current info elements, got %d",
            g_GameFlow.level_count, current_arr->length);
        goto cleanup;
    }
    for (int i = 0; i < (signed)current_arr->length; i++) {
        struct json_object_s *cur_obj = json_array_get_object(current_arr, i);
        if (!cur_obj) {
            LOG_ERROR("Malformed save: invalid current info");
            goto cleanup;
        }
        RESUME_INFO *current = &game_info->current[i];
        json_object_evict_key(cur_obj, "deaths");
        json_object_append_int(cur_obj, "deaths", current->stats.death_count);
    }

    File_Seek(fp, 0, FILE_SEEK_SET);
    SaveGame_BSON_SaveRaw(fp, root, version);
    ret = true;

cleanup:
    json_value_free(root);
    return ret;
}
