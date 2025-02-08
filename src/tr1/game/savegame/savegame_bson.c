#include "game/savegame/savegame_bson.h"

#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/stats.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/bson.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/json.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <inttypes.h>
#include <stdio.h>
#include <zconf.h>
#include <zlib.h>

#define SAVEGAME_BSON_MAGIC MKTAG('T', '1', 'M', 'B')

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    int16_t initial_version;
    uint16_t version;
    int32_t compressed_size;
    int32_t uncompressed_size;
} SAVEGAME_BSON_HEADER;
#pragma pack(pop)

typedef struct {
    int16_t count;
    int16_t id_map[NUM_EFFECTS];
} SAVEGAME_BSON_FX_ORDER;

static void M_SaveRaw(MYFILE *fp, JSON_VALUE *root, int32_t version);
static JSON_VALUE *M_ParseFromBuffer(
    const char *buffer, size_t buffer_size, int32_t *version_out);
static JSON_VALUE *M_ParseFromFile(MYFILE *fp, int32_t *version_out);
static bool M_LoadResumeInfo(JSON_ARRAY *levels_arr, RESUME_INFO *resume_info);
static bool M_LoadDiscontinuedStartInfo(
    JSON_ARRAY *start_arr, GAME_INFO *game_info);
static bool M_LoadDiscontinuedEndInfo(
    JSON_ARRAY *end_arr, GAME_INFO *game_info);
static bool M_LoadMisc(
    JSON_OBJECT *misc_obj, GAME_INFO *game_info, uint16_t header_version);
static bool M_LoadInventory(JSON_OBJECT *inv_obj);
static bool M_LoadFlipmaps(JSON_OBJECT *flipmap_obj);
static bool M_LoadCameras(JSON_ARRAY *cameras_arr);
static bool M_LoadItems(JSON_ARRAY *items_arr, uint16_t header_version);
static bool M_LoadEffects(JSON_ARRAY *fx_arr);
static bool M_LoadArm(JSON_OBJECT *arm_obj, LARA_ARM *arm);
static bool M_LoadAmmo(JSON_OBJECT *ammo_obj, AMMO_INFO *ammo);
static bool M_LoadLOT(JSON_OBJECT *lot_obj, LOT_INFO *lot);
static bool M_LoadLara(
    JSON_OBJECT *lara_obj, LARA_INFO *lara, uint16_t header_version);
static bool M_LoadCurrentMusic(JSON_OBJECT *music_obj);
static bool M_LoadMusicTrackFlags(JSON_ARRAY *music_track_arr);
static JSON_ARRAY *M_DumpResumeInfo(RESUME_INFO *game_info);
static JSON_OBJECT *M_DumpMisc(GAME_INFO *game_info);
static JSON_OBJECT *M_DumpInventory(void);
static JSON_OBJECT *M_DumpFlipmaps(void);
static JSON_ARRAY *M_DumpCameras(void);
static JSON_ARRAY *M_DumpItems(void);
static JSON_ARRAY *M_DumpEffects(void);
static JSON_OBJECT *M_DumpArm(LARA_ARM *arm);
static JSON_OBJECT *M_DumpAmmo(AMMO_INFO *ammo);
static JSON_OBJECT *M_DumpLOT(LOT_INFO *lot);
static JSON_OBJECT *M_DumpLara(LARA_INFO *lara);
static JSON_OBJECT *M_DumpCurrentMusic(void);
static JSON_ARRAY *M_DumpMusicTrackFlags(void);

static void M_GetFXOrder(SAVEGAME_BSON_FX_ORDER *order);
static bool M_IsValidItemObject(
    GAME_OBJECT_ID saved_obj_id, GAME_OBJECT_ID current_obj_id);

static void M_SaveRaw(MYFILE *fp, JSON_VALUE *root, int32_t version)
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

    SAVEGAME_BSON_HEADER header = {
        .magic = SAVEGAME_BSON_MAGIC,
        .initial_version = g_GameInfo.save_initial_version,
        .version = version,
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };

    File_WriteData(fp, &header, sizeof(header));
    File_WriteData(fp, compressed, compressed_size);

    Memory_FreePointer(&compressed);
}

static void M_GetFXOrder(SAVEGAME_BSON_FX_ORDER *order)
{
    order->count = 0;
    for (int i = 0; i < NUM_EFFECTS; i++) {
        order->id_map[i] = -1;
    }

    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        order->id_map[link_num] = order->count;
        order->count++;
    }
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
        case O_PISTOL_AMMO_ITEM: return initial_obj_id == O_PISTOL_ANIM;
        case O_SG_AMMO_ITEM: return initial_obj_id == O_SHOTGUN_ITEM;
        case O_MAG_AMMO_ITEM: return initial_obj_id == O_MAGNUM_ITEM;
        case O_UZI_AMMO_ITEM: return initial_obj_id == O_UZI_ITEM;
        // dual-state animals
        case O_ALLIGATOR: return initial_obj_id == O_CROCODILE;
        case O_CROCODILE: return initial_obj_id == O_ALLIGATOR;
        case O_RAT: return initial_obj_id == O_VOLE;
        case O_VOLE: return initial_obj_id == O_RAT;
        // default
        default: return false;
    }
    // clang-format on
}

static JSON_VALUE *M_ParseFromBuffer(
    const char *buffer, size_t buffer_size, int32_t *version_out)
{
    SAVEGAME_BSON_HEADER *header = (SAVEGAME_BSON_HEADER *)buffer;
    if (header->magic != SAVEGAME_BSON_MAGIC) {
        LOG_ERROR("Invalid savegame magic");
        return nullptr;
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
        return nullptr;
    }

    JSON_VALUE *root = BSON_Parse(uncompressed, uncompressed_size);
    Memory_FreePointer(&uncompressed);
    return root;
}

static JSON_VALUE *M_ParseFromFile(MYFILE *fp, int32_t *version_out)
{
    const size_t buffer_size = File_Size(fp);
    char *buffer = Memory_Alloc(buffer_size);
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, buffer, buffer_size);

    JSON_VALUE *ret = M_ParseFromBuffer(buffer, buffer_size, version_out);
    Memory_FreePointer(&buffer);
    return ret;
}

static bool M_LoadResumeInfo(JSON_ARRAY *resume_arr, RESUME_INFO *resume_info)
{
    ASSERT(resume_info != nullptr);
    if (!resume_arr) {
        LOG_ERROR("Malformed save: invalid or missing resume array");
        return false;
    }
    if ((signed)resume_arr->length != GF_GetLevelTable(GFLT_MAIN)->count) {
        LOG_ERROR(
            "Malformed save: expected %d resume info elements, got %d",
            GF_GetLevelTable(GFLT_MAIN)->count, resume_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)resume_arr->length; i++) {
        JSON_OBJECT *resume_obj = JSON_ArrayGetObject(resume_arr, i);
        if (!resume_obj) {
            LOG_ERROR("Malformed save: invalid resume info");
            return false;
        }
        RESUME_INFO *resume = &resume_info[i];
        resume->lara_hitpoints = JSON_ObjectGetInt(
            resume_obj, "lara_hitpoints",
            g_Config.gameplay.start_lara_hitpoints);
        resume->pistol_ammo = JSON_ObjectGetInt(resume_obj, "pistol_ammo", 0);
        resume->magnum_ammo = JSON_ObjectGetInt(resume_obj, "magnum_ammo", 0);
        resume->uzi_ammo = JSON_ObjectGetInt(resume_obj, "uzi_ammo", 0);
        resume->shotgun_ammo = JSON_ObjectGetInt(resume_obj, "shotgun_ammo", 0);
        resume->num_medis = JSON_ObjectGetInt(resume_obj, "num_medis", 0);
        resume->num_big_medis =
            JSON_ObjectGetInt(resume_obj, "num_big_medis", 0);
        resume->num_scions = JSON_ObjectGetInt(resume_obj, "num_scions", 0);
        resume->gun_status = JSON_ObjectGetInt(resume_obj, "gun_status", 0);
        resume->equipped_gun_type =
            JSON_ObjectGetInt(resume_obj, "gun_type", LGT_UNARMED);
        resume->holsters_gun_type =
            JSON_ObjectGetInt(resume_obj, "holsters_gun_type", LGT_UNKNOWN);
        resume->back_gun_type =
            JSON_ObjectGetInt(resume_obj, "back_gun_type", LGT_UNKNOWN);
        resume->flags.available =
            JSON_ObjectGetBool(resume_obj, "available", 0);
        resume->flags.got_pistols =
            JSON_ObjectGetBool(resume_obj, "got_pistols", 0);
        resume->flags.got_magnums =
            JSON_ObjectGetBool(resume_obj, "got_magnums", 0);
        resume->flags.got_uzis = JSON_ObjectGetBool(resume_obj, "got_uzis", 0);
        resume->flags.got_shotgun =
            JSON_ObjectGetBool(resume_obj, "got_shotgun", 0);
        resume->flags.costume = JSON_ObjectGetBool(resume_obj, "costume", 0);

        resume->stats.timer =
            JSON_ObjectGetInt(resume_obj, "timer", resume->stats.timer);
        resume->stats.secret_flags = JSON_ObjectGetInt(
            resume_obj, "secrets", resume->stats.secret_flags);
        Stats_UpdateSecrets(&resume->stats);
        resume->stats.kill_count =
            JSON_ObjectGetInt(resume_obj, "kills", resume->stats.kill_count);
        resume->stats.pickup_count = JSON_ObjectGetInt(
            resume_obj, "pickups", resume->stats.pickup_count);
        resume->stats.max_secret_count = JSON_ObjectGetInt(
            resume_obj, "max_secrets", resume->stats.max_secret_count);
        resume->stats.max_kill_count = JSON_ObjectGetInt(
            resume_obj, "max_kills", resume->stats.max_kill_count);
        resume->stats.max_pickup_count = JSON_ObjectGetInt(
            resume_obj, "max_pickups", resume->stats.max_pickup_count);
    }
    return true;
}

static bool M_LoadDiscontinuedStartInfo(
    JSON_ARRAY *start_arr, GAME_INFO *game_info)
{
    // This function solely exists for backward compatibility with 2.6 and 2.7
    // saves.
    ASSERT(game_info != nullptr);
    ASSERT(game_info->current != nullptr);
    if (!start_arr) {
        LOG_ERROR(
            "Malformed save: invalid or missing discontinued start array");
        return false;
    }
    if ((signed)start_arr->length != GF_GetLevelTable(GFLT_MAIN)->count) {
        LOG_ERROR(
            "Malformed save: expected %d start info elements, got %d",
            GF_GetLevelTable(GFLT_MAIN)->count, start_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)start_arr->length; i++) {
        JSON_OBJECT *start_obj = JSON_ArrayGetObject(start_arr, i);
        if (!start_obj) {
            LOG_ERROR("Malformed save: invalid discontinued start info");
            return false;
        }
        RESUME_INFO *start = &game_info->current[i];
        start->lara_hitpoints = JSON_ObjectGetInt(
            start_obj, "lara_hitpoints",
            g_Config.gameplay.start_lara_hitpoints);
        start->pistol_ammo = JSON_ObjectGetInt(start_obj, "pistol_ammo", 0);
        start->magnum_ammo = JSON_ObjectGetInt(start_obj, "magnum_ammo", 0);
        start->uzi_ammo = JSON_ObjectGetInt(start_obj, "uzi_ammo", 0);
        start->shotgun_ammo = JSON_ObjectGetInt(start_obj, "shotgun_ammo", 0);
        start->num_medis = JSON_ObjectGetInt(start_obj, "num_medis", 0);
        start->num_big_medis = JSON_ObjectGetInt(start_obj, "num_big_medis", 0);
        start->num_scions = JSON_ObjectGetInt(start_obj, "num_scions", 0);
        start->gun_status = JSON_ObjectGetInt(start_obj, "gun_status", 0);
        start->equipped_gun_type =
            JSON_ObjectGetInt(start_obj, "gun_type", LGT_UNARMED);
        start->holsters_gun_type = LGT_UNKNOWN;
        start->back_gun_type = LGT_UNKNOWN;
        start->flags.available = JSON_ObjectGetBool(start_obj, "available", 0);
        start->flags.got_pistols =
            JSON_ObjectGetBool(start_obj, "got_pistols", 0);
        start->flags.got_magnums =
            JSON_ObjectGetBool(start_obj, "got_magnums", 0);
        start->flags.got_uzis = JSON_ObjectGetBool(start_obj, "got_uzis", 0);
        start->flags.got_shotgun =
            JSON_ObjectGetBool(start_obj, "got_shotgun", 0);
        start->flags.costume = JSON_ObjectGetBool(start_obj, "costume", 0);
    }
    return true;
}

static bool M_LoadDiscontinuedEndInfo(JSON_ARRAY *end_arr, GAME_INFO *game_info)
{
    // This function solely exists for backward compatibility with 2.6 and 2.7
    // saves.
    ASSERT(game_info != nullptr);
    ASSERT(game_info->current != nullptr);
    if (!end_arr) {
        LOG_ERROR("Malformed save: invalid or missing resume info array");
        return false;
    }
    if ((signed)end_arr->length != GF_GetLevelTable(GFLT_MAIN)->count) {
        LOG_ERROR(
            "Malformed save: expected %d resume info elements, got %d",
            GF_GetLevelTable(GFLT_MAIN)->count, end_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)end_arr->length; i++) {
        JSON_OBJECT *end_obj = JSON_ArrayGetObject(end_arr, i);
        if (!end_obj) {
            LOG_ERROR("Malformed save: invalid resume info");
            return false;
        }
        LEVEL_STATS *end = &game_info->current[i].stats;
        end->timer = JSON_ObjectGetInt(end_obj, "timer", end->timer);
        end->secret_flags =
            JSON_ObjectGetInt(end_obj, "secrets", end->secret_flags);
        Stats_UpdateSecrets(end);
        end->kill_count = JSON_ObjectGetInt(end_obj, "kills", end->kill_count);
        end->pickup_count =
            JSON_ObjectGetInt(end_obj, "pickups", end->pickup_count);
        end->max_secret_count =
            JSON_ObjectGetInt(end_obj, "max_secrets", end->max_secret_count);
        end->max_kill_count =
            JSON_ObjectGetInt(end_obj, "max_kills", end->max_kill_count);
        end->max_pickup_count =
            JSON_ObjectGetInt(end_obj, "max_pickups", end->max_pickup_count);
    }
    return true;
}

static bool M_LoadMisc(
    JSON_OBJECT *misc_obj, GAME_INFO *game_info, uint16_t header_version)
{
    ASSERT(game_info != nullptr);
    if (!misc_obj) {
        LOG_ERROR("Malformed save: invalid or missing misc info");
        return false;
    }
    game_info->bonus_flag = JSON_ObjectGetInt(misc_obj, "bonus_flag", 0);
    if (header_version >= VERSION_4) {
        game_info->bonus_level_unlock =
            JSON_ObjectGetBool(misc_obj, "bonus_level_unlock", 0);
        game_info->death_count = JSON_ObjectGetInt(misc_obj, "death_count", -1);
    }
    return true;
}

static bool M_LoadInventory(JSON_OBJECT *inv_obj)
{
    if (!inv_obj) {
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
    Inv_AddItemNTimes(O_LEADBAR_ITEM, JSON_ObjectGetInt(inv_obj, "leadbar", 0));
    return true;
}

static bool M_LoadFlipmaps(JSON_OBJECT *flipmap_obj)
{
    if (!flipmap_obj) {
        LOG_ERROR("Malformed save: invalid or missing flipmap info");
        return false;
    }

    if (JSON_ObjectGetBool(flipmap_obj, "status", false)) {
        Room_FlipMap();
    }

    g_FlipEffect = JSON_ObjectGetInt(flipmap_obj, "effect", 0);
    g_FlipTimer = JSON_ObjectGetInt(flipmap_obj, "timer", 0);

    JSON_ARRAY *flipmap_arr = JSON_ObjectGetArray(flipmap_obj, "table");
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
        g_FlipMapTable[i] = JSON_ArrayGetInt(flipmap_arr, i, 0) << 8;
    }

    return true;
}

static bool M_LoadCameras(JSON_ARRAY *cameras_arr)
{
    if (!cameras_arr) {
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

static bool M_LoadItems(JSON_ARRAY *items_arr, uint16_t header_version)
{
    if (!items_arr) {
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

    for (int32_t i = 0; i < item_count; i++) {
        JSON_OBJECT *item_obj = JSON_ArrayGetObject(items_arr, i);
        if (!item_obj) {
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
            item->pos.x = JSON_ObjectGetInt(item_obj, "x", item->pos.x);
            item->pos.y = JSON_ObjectGetInt(item_obj, "y", item->pos.y);
            item->pos.z = JSON_ObjectGetInt(item_obj, "z", item->pos.z);
            item->rot.x = JSON_ObjectGetInt(item_obj, "x_rot", item->rot.x);
            item->rot.y = JSON_ObjectGetInt(item_obj, "y_rot", item->rot.y);
            item->rot.z = JSON_ObjectGetInt(item_obj, "z_rot", item->rot.z);
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

            if (item->flags & IF_KILLED) {
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
                LOT_EnableBaddieAI(i, 1);
                CREATURE *creature = item->data;
                if (creature) {
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
            }

            if (header_version >= VERSION_3
                && item->object_id == O_FLAME_EMITTER
                && g_Config.gameplay.enable_enhanced_saves) {
                int32_t effect_num = JSON_ObjectGetInt(item_obj, "fx_num", -1);
                if (effect_num != -1) {
                    item->data = (void *)(intptr_t)(effect_num + 1);
                }
            }

            if (header_version >= VERSION_5
                && item->object_id == O_BACON_LARA) {
                const int32_t status =
                    JSON_ObjectGetInt(item_obj, "bl_status", 0);
                item->data = (void *)(intptr_t)status;
            }
        }

        JSON_ARRAY *carried_items =
            JSON_ObjectGetArray(item_obj, "carried_items");
        if (carried_items) {
            CARRIED_ITEM *carried_item = item->carried_item;
            for (int j = 0; j < (signed)carried_items->length; j++) {
                if (!carried_item) {
                    LOG_ERROR("Malformed save: carried item mismatch");
                    return false;
                }

                JSON_OBJECT *carried_item_obj =
                    JSON_ArrayGetObject(carried_items, j);

                carried_item->object_id = JSON_ObjectGetInt(
                    carried_item_obj, "object_id", carried_item->object_id);
                carried_item->pos.x = JSON_ObjectGetInt(
                    carried_item_obj, "x", carried_item->pos.x);
                carried_item->pos.y = JSON_ObjectGetInt(
                    carried_item_obj, "y", carried_item->pos.y);
                carried_item->pos.z = JSON_ObjectGetInt(
                    carried_item_obj, "z", carried_item->pos.z);
                carried_item->rot.y = JSON_ObjectGetInt(
                    carried_item_obj, "y_rot", carried_item->rot.y);
                carried_item->room_num = JSON_ObjectGetInt(
                    carried_item_obj, "room_num", carried_item->room_num);
                carried_item->fall_speed = JSON_ObjectGetInt(
                    carried_item_obj, "fall_speed", carried_item->fall_speed);
                carried_item->status = JSON_ObjectGetInt(
                    carried_item_obj, "status", carried_item->status);

                carried_item = carried_item->next_item;
            }

            Carrier_TestItemDrops(i);
        } else if (header_version < VERSION_4) {
            Carrier_TestLegacyDrops(i);
        }
    }

    return true;
}

static bool M_LoadEffects(JSON_ARRAY *fx_arr)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return true;
    }

    if (!fx_arr) {
        LOG_ERROR("Malformed save: invalid or missing effect array");
        return false;
    }

    if ((signed)fx_arr->length >= NUM_EFFECTS) {
        LOG_WARNING(
            "Malformed save: expected a max of %d effect, got %d. effect over "
            "the "
            "maximum will not be created.",
            NUM_EFFECTS - 1, fx_arr->length);
    }

    for (int i = 0; i < (signed)fx_arr->length; i++) {
        JSON_OBJECT *fx_obj = JSON_ArrayGetObject(fx_arr, i);
        if (!fx_obj) {
            LOG_ERROR("Malformed save: invalid effect data");
            return false;
        }

        int32_t x = JSON_ObjectGetInt(fx_obj, "x", 0);
        int32_t y = JSON_ObjectGetInt(fx_obj, "y", 0);
        int32_t z = JSON_ObjectGetInt(fx_obj, "z", 0);
        int16_t room_num = JSON_ObjectGetInt(fx_obj, "room_number", 0);
        GAME_OBJECT_ID obj_id = JSON_ObjectGetInt(fx_obj, "object_number", 0);
        int16_t speed = JSON_ObjectGetInt(fx_obj, "speed", 0);
        int16_t fall_speed = JSON_ObjectGetInt(fx_obj, "fall_speed", 0);
        int16_t frame_num = JSON_ObjectGetInt(fx_obj, "frame_number", 0);
        int16_t counter = JSON_ObjectGetInt(fx_obj, "counter", 0);
        int16_t shade = JSON_ObjectGetInt(fx_obj, "shade", 0);

        int16_t effect_num = Effect_Create(room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *effect = Effect_Get(effect_num);
            effect->pos.x = x;
            effect->pos.y = y;
            effect->pos.z = z;
            effect->object_id = obj_id;
            effect->speed = speed;
            effect->fall_speed = fall_speed;
            effect->frame_num = frame_num;
            effect->counter = counter;
            effect->shade = shade;
        }
    }

    return true;
}

static bool M_LoadArm(JSON_OBJECT *arm_obj, LARA_ARM *arm)
{
    ASSERT(arm != nullptr);
    if (!arm_obj) {
        LOG_ERROR("Malformed save: invalid or missing arm info");
        return false;
    }

    arm->frame_num = JSON_ObjectGetInt(arm_obj, "frame_num", arm->frame_num);
    arm->lock = JSON_ObjectGetInt(arm_obj, "lock", arm->lock);
    arm->rot.x = JSON_ObjectGetInt(arm_obj, "x_rot", arm->rot.x);
    arm->rot.y = JSON_ObjectGetInt(arm_obj, "y_rot", arm->rot.y);
    arm->rot.z = JSON_ObjectGetInt(arm_obj, "z_rot", arm->rot.z);
    arm->flash_gun = JSON_ObjectGetInt(arm_obj, "flash_gun", arm->flash_gun);
    return true;
}

static bool M_LoadAmmo(JSON_OBJECT *ammo_obj, AMMO_INFO *ammo)
{
    ASSERT(ammo != nullptr);
    if (!ammo_obj) {
        LOG_ERROR("Malformed save: invalid or missing ammo info");
        return false;
    }

    ammo->ammo = JSON_ObjectGetInt(ammo_obj, "ammo", ammo->ammo);
    ammo->hit = JSON_ObjectGetInt(ammo_obj, "hit", ammo->hit);
    ammo->miss = JSON_ObjectGetInt(ammo_obj, "miss", ammo->miss);
    return true;
}

static bool M_LoadLOT(JSON_OBJECT *lot_obj, LOT_INFO *lot)
{
    ASSERT(lot != nullptr);
    if (!lot_obj) {
        LOG_ERROR("Malformed save: invalid or missing LOT info");
        return false;
    }

    lot->head = JSON_ObjectGetInt(lot_obj, "head", lot->head);
    lot->tail = JSON_ObjectGetInt(lot_obj, "tail", lot->tail);
    lot->search_num = JSON_ObjectGetInt(lot_obj, "search_num", lot->search_num);
    lot->block_mask = JSON_ObjectGetInt(lot_obj, "block_mask", lot->block_mask);
    lot->step = JSON_ObjectGetInt(lot_obj, "step", lot->step);
    lot->drop = JSON_ObjectGetInt(lot_obj, "drop", lot->drop);
    lot->fly = JSON_ObjectGetInt(lot_obj, "fly", lot->fly);
    lot->zone_count = JSON_ObjectGetInt(lot_obj, "zone_count", lot->zone_count);
    lot->target_box = JSON_ObjectGetInt(lot_obj, "target_box", lot->target_box);
    lot->required_box =
        JSON_ObjectGetInt(lot_obj, "required_box", lot->required_box);
    lot->target.x = JSON_ObjectGetInt(lot_obj, "x", lot->target.x);
    lot->target.y = JSON_ObjectGetInt(lot_obj, "y", lot->target.y);
    lot->target.z = JSON_ObjectGetInt(lot_obj, "z", lot->target.z);
    return true;
}

static bool M_LoadLara(
    JSON_OBJECT *lara_obj, LARA_INFO *lara, uint16_t header_version)
{
    ASSERT(lara != nullptr);
    if (!lara_obj) {
        LOG_ERROR("Malformed save: invalid or missing Lara info");
        return false;
    }

    lara->item_num = JSON_ObjectGetInt(lara_obj, "item_number", lara->item_num);
    lara->gun_status =
        JSON_ObjectGetInt(lara_obj, "gun_status", lara->gun_status);
    lara->gun_type = JSON_ObjectGetInt(lara_obj, "gun_type", lara->gun_type);
    lara->request_gun_type =
        JSON_ObjectGetInt(lara_obj, "request_gun_type", lara->request_gun_type);
    lara->calc_fall_speed =
        JSON_ObjectGetInt(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    lara->water_status =
        JSON_ObjectGetInt(lara_obj, "water_status", lara->water_status);
    lara->pose_count =
        JSON_ObjectGetInt(lara_obj, "pose_count", lara->pose_count);
    lara->hit_frame = JSON_ObjectGetInt(lara_obj, "hit_frame", lara->hit_frame);
    lara->hit_direction =
        JSON_ObjectGetInt(lara_obj, "hit_direction", lara->hit_direction);
    lara->air = JSON_ObjectGetInt(lara_obj, "air", lara->air);
    lara->dive_timer =
        JSON_ObjectGetInt(lara_obj, "dive_count", lara->dive_timer);
    lara->death_timer =
        JSON_ObjectGetInt(lara_obj, "death_count", lara->death_timer);
    lara->current_active =
        JSON_ObjectGetInt(lara_obj, "current_active", lara->current_active);

    lara->hit_effect_count =
        JSON_ObjectGetInt(lara_obj, "hit_effect_count", lara->hit_effect_count);
    const int32_t hit_effect = JSON_ObjectGetInt(lara_obj, "hit_effect", 0);
    lara->hit_effect = hit_effect && g_Config.gameplay.enable_enhanced_saves
        ? Effect_Get(hit_effect)
        : nullptr;

    lara->mesh_effects =
        JSON_ObjectGetInt(lara_obj, "mesh_effects", lara->mesh_effects);

    JSON_ARRAY *lara_meshes_arr = JSON_ObjectGetArray(lara_obj, "meshes");
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
    lara->head_rot.y =
        JSON_ObjectGetInt(lara_obj, "head_rot.y", lara->head_rot.y);
    lara->head_rot.x =
        JSON_ObjectGetInt(lara_obj, "head_rot.x", lara->head_rot.x);
    lara->head_rot.z =
        JSON_ObjectGetInt(lara_obj, "head_rot.z", lara->head_rot.z);
    lara->torso_rot.y =
        JSON_ObjectGetInt(lara_obj, "torso_rot.y", lara->torso_rot.y);
    lara->torso_rot.x =
        JSON_ObjectGetInt(lara_obj, "torso_rot.x", lara->torso_rot.x);
    lara->torso_rot.z =
        JSON_ObjectGetInt(lara_obj, "torso_rot.z", lara->torso_rot.z);

    if (!M_LoadArm(
            JSON_ObjectGetObject(lara_obj, "left_arm"), &lara->left_arm)) {
        return false;
    }

    if (!M_LoadArm(
            JSON_ObjectGetObject(lara_obj, "right_arm"), &lara->right_arm)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "pistols"), &lara->pistols)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "magnums"), &lara->magnums)) {
        return false;
    }

    if (!M_LoadAmmo(JSON_ObjectGetObject(lara_obj, "uzis"), &lara->uzis)) {
        return false;
    }

    if (!M_LoadAmmo(
            JSON_ObjectGetObject(lara_obj, "shotgun"), &lara->shotgun)) {
        return false;
    }

    if (!M_LoadLOT(JSON_ObjectGetObject(lara_obj, "lot"), &lara->lot)) {
        return false;
    }

    if (header_version >= VERSION_6) {
        lara->interact_target.item_num = JSON_ObjectGetInt(
            lara_obj, "interact_target.item_num",
            lara->interact_target.item_num);
        lara->interact_target.move_count = JSON_ObjectGetInt(
            lara_obj, "interact_target.move_count",
            lara->interact_target.move_count);
        lara->interact_target.is_moving = JSON_ObjectGetBool(
            lara_obj, "interact_target.is_moving",
            lara->interact_target.is_moving);
    }

    return true;
}

static bool M_LoadCurrentMusic(JSON_OBJECT *music_obj)
{
    if (g_Config.audio.music_load_condition == MUSIC_LOAD_NEVER) {
        return true;
    }

    if (!music_obj) {
        LOG_WARNING("Malformed save: invalid or missing current music");
        return true;
    }

    int16_t current_track = JSON_ObjectGetInt(music_obj, "current_track", -1);
    double timestamp = JSON_ObjectGetDouble(music_obj, "timestamp", -1.0);
    if (current_track != MX_INACTIVE) {
        const bool is_ambient =
            JSON_ObjectGetBool(music_obj, "is_ambient", false);
        if (is_ambient) {
            if (g_Config.audio.music_load_condition == MUSIC_LOAD_NON_AMBIENT) {
                return true;
            }
            Music_Play(current_track, MPM_LOOPED);
        } else {
            Music_Play(current_track, MPM_ALWAYS);
        }

        if (!Music_SeekTimestamp(timestamp)) {
            LOG_WARNING(
                "Could not load current track %d at timestamp %" PRId64 ".",
                current_track, timestamp);
        }
    }
    return true;
}

static bool M_LoadMusicTrackFlags(JSON_ARRAY *music_track_arr)
{
    if (!g_Config.audio.load_music_triggers) {
        return true;
    }

    if (!music_track_arr) {
        LOG_WARNING("Malformed save: invalid or missing music track array");
        return true;
    }

    if ((signed)music_track_arr->length != MAX_MUSIC_TRACKS) {
        LOG_WARNING(
            "Malformed save: expected %d music track flags, got %d",
            MAX_MUSIC_TRACKS, music_track_arr->length);
        return true;
    }

    for (int32_t i = 0; i < (signed)music_track_arr->length; i++) {
        Music_SetTrackFlags(i, JSON_ArrayGetInt(music_track_arr, i, 0));
    }

    return true;
}

static JSON_ARRAY *M_DumpResumeInfo(RESUME_INFO *resume_info)
{
    JSON_ARRAY *resume_arr = JSON_ArrayNew();
    ASSERT(resume_info != nullptr);
    for (int i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
        RESUME_INFO *resume = &resume_info[i];
        JSON_OBJECT *resume_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(
            resume_obj, "lara_hitpoints", resume->lara_hitpoints);
        JSON_ObjectAppendInt(resume_obj, "pistol_ammo", resume->pistol_ammo);
        JSON_ObjectAppendInt(resume_obj, "magnum_ammo", resume->magnum_ammo);
        JSON_ObjectAppendInt(resume_obj, "uzi_ammo", resume->uzi_ammo);
        JSON_ObjectAppendInt(resume_obj, "shotgun_ammo", resume->shotgun_ammo);
        JSON_ObjectAppendInt(resume_obj, "num_medis", resume->num_medis);
        JSON_ObjectAppendInt(
            resume_obj, "num_big_medis", resume->num_big_medis);
        JSON_ObjectAppendInt(resume_obj, "num_scions", resume->num_scions);
        JSON_ObjectAppendInt(resume_obj, "gun_status", resume->gun_status);
        JSON_ObjectAppendInt(resume_obj, "gun_type", resume->equipped_gun_type);
        JSON_ObjectAppendInt(
            resume_obj, "holsters_gun_type", resume->holsters_gun_type);
        JSON_ObjectAppendInt(
            resume_obj, "back_gun_type", resume->back_gun_type);
        JSON_ObjectAppendBool(resume_obj, "available", resume->flags.available);
        JSON_ObjectAppendBool(
            resume_obj, "got_pistols", resume->flags.got_pistols);
        JSON_ObjectAppendBool(
            resume_obj, "got_magnums", resume->flags.got_magnums);
        JSON_ObjectAppendBool(resume_obj, "got_uzis", resume->flags.got_uzis);
        JSON_ObjectAppendBool(
            resume_obj, "got_shotgun", resume->flags.got_shotgun);
        JSON_ObjectAppendBool(resume_obj, "costume", resume->flags.costume);
        JSON_ObjectAppendInt(resume_obj, "timer", resume->stats.timer);
        JSON_ObjectAppendInt(resume_obj, "kills", resume->stats.kill_count);
        JSON_ObjectAppendInt(resume_obj, "secrets", resume->stats.secret_flags);
        JSON_ObjectAppendInt(resume_obj, "pickups", resume->stats.pickup_count);
        JSON_ObjectAppendInt(
            resume_obj, "max_kills", resume->stats.max_kill_count);
        JSON_ObjectAppendInt(
            resume_obj, "max_secrets", resume->stats.max_secret_count);
        JSON_ObjectAppendInt(
            resume_obj, "max_pickups", resume->stats.max_pickup_count);
        JSON_ArrayAppendObject(resume_arr, resume_obj);
    }
    return resume_arr;
}

static JSON_OBJECT *M_DumpMisc(GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);
    JSON_OBJECT *misc_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(misc_obj, "bonus_flag", game_info->bonus_flag);
    JSON_ObjectAppendBool(
        misc_obj, "bonus_level_unlock", game_info->bonus_level_unlock);
    JSON_ObjectAppendInt(misc_obj, "death_count", game_info->death_count);
    return misc_obj;
}

static JSON_OBJECT *M_DumpInventory(void)
{
    JSON_OBJECT *inv_obj = JSON_ObjectNew();
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
    JSON_ObjectAppendInt(inv_obj, "leadbar", Inv_RequestItem(O_LEADBAR_ITEM));
    return inv_obj;
}

static JSON_OBJECT *M_DumpFlipmaps(void)
{
    JSON_OBJECT *flipmap_obj = JSON_ObjectNew();
    JSON_ObjectAppendBool(flipmap_obj, "status", Room_GetFlipStatus());
    JSON_ObjectAppendInt(flipmap_obj, "effect", g_FlipEffect);
    JSON_ObjectAppendInt(flipmap_obj, "timer", g_FlipTimer);
    JSON_ARRAY *flipmap_arr = JSON_ArrayNew();
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        JSON_ArrayAppendInt(flipmap_arr, g_FlipMapTable[i] >> 8);
    }
    JSON_ObjectAppendArray(flipmap_obj, "table", flipmap_arr);
    return flipmap_obj;
}

static JSON_ARRAY *M_DumpCameras(void)
{
    JSON_ARRAY *cameras_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        JSON_ArrayAppendInt(cameras_arr, object->flags);
    }
    return cameras_arr;
}

static JSON_ARRAY *M_DumpItems(void)
{
    Savegame_ProcessItemsBeforeSave();

    SAVEGAME_BSON_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    JSON_ARRAY *items_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        JSON_OBJECT *item_obj = JSON_ObjectNew();
        const ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);

        JSON_ObjectAppendInt(item_obj, "obj_num", item->object_id);

        if (obj->save_position) {
            JSON_ObjectAppendInt(item_obj, "x", item->pos.x);
            JSON_ObjectAppendInt(item_obj, "y", item->pos.y);
            JSON_ObjectAppendInt(item_obj, "z", item->pos.z);
            JSON_ObjectAppendInt(item_obj, "x_rot", item->rot.x);
            JSON_ObjectAppendInt(item_obj, "y_rot", item->rot.y);
            JSON_ObjectAppendInt(item_obj, "z_rot", item->rot.z);
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
            if (obj->intelligent && item->data) {
                CREATURE *creature = item->data;
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

            if (item->object_id == O_FLAME_EMITTER && item->data) {
                int32_t effect_num = (int32_t)(intptr_t)item->data - 1;
                effect_num = fx_order.id_map[effect_num];
                JSON_ObjectAppendInt(item_obj, "fx_num", effect_num);
            }

            if (item->object_id == O_BACON_LARA && item->data) {
                const int32_t status = (int32_t)(intptr_t)item->data;
                JSON_ObjectAppendInt(item_obj, "bl_status", status);
            }
        }

        JSON_ARRAY *carried_items_arr = JSON_ArrayNew();

        const CARRIED_ITEM *drop_item = item->carried_item;
        while (drop_item) {
            JSON_OBJECT *drop_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(drop_obj, "object_id", drop_item->object_id);
            JSON_ObjectAppendInt(drop_obj, "x", drop_item->pos.x);
            JSON_ObjectAppendInt(drop_obj, "y", drop_item->pos.y);
            JSON_ObjectAppendInt(drop_obj, "z", drop_item->pos.z);
            JSON_ObjectAppendInt(drop_obj, "y_rot", drop_item->rot.y);
            JSON_ObjectAppendInt(drop_obj, "room_num", drop_item->room_num);
            JSON_ObjectAppendInt(drop_obj, "fall_speed", drop_item->fall_speed);

            DROP_STATUS status = Carrier_GetSaveStatus(drop_item);
            JSON_ObjectAppendInt(drop_obj, "status", status);

            JSON_ArrayAppendObject(carried_items_arr, drop_obj);
            drop_item = drop_item->next_item;
        }

        JSON_ObjectAppendArray(item_obj, "carried_items", carried_items_arr);

        JSON_ArrayAppendObject(items_arr, item_obj);
    }
    return items_arr;
}

static JSON_ARRAY *M_DumpEffects(void)
{
    JSON_ARRAY *fx_arr = JSON_ArrayNew();

    SAVEGAME_BSON_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        JSON_OBJECT *fx_obj = JSON_ObjectNew();
        EFFECT *effect = Effect_Get(link_num);
        JSON_ObjectAppendInt(fx_obj, "x", effect->pos.x);
        JSON_ObjectAppendInt(fx_obj, "y", effect->pos.y);
        JSON_ObjectAppendInt(fx_obj, "z", effect->pos.z);
        JSON_ObjectAppendInt(fx_obj, "room_number", effect->room_num);
        JSON_ObjectAppendInt(fx_obj, "object_number", effect->object_id);
        JSON_ObjectAppendInt(fx_obj, "speed", effect->speed);
        JSON_ObjectAppendInt(fx_obj, "fall_speed", effect->fall_speed);
        JSON_ObjectAppendInt(fx_obj, "frame_number", effect->frame_num);
        JSON_ObjectAppendInt(fx_obj, "counter", effect->counter);
        JSON_ObjectAppendInt(fx_obj, "shade", effect->shade);
        JSON_ArrayAppendObject(fx_arr, fx_obj);
    }

    return fx_arr;
}

static JSON_OBJECT *M_DumpArm(LARA_ARM *arm)
{
    ASSERT(arm != nullptr);
    JSON_OBJECT *arm_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(arm_obj, "frame_num", arm->frame_num);
    JSON_ObjectAppendInt(arm_obj, "lock", arm->lock);
    JSON_ObjectAppendInt(arm_obj, "x_rot", arm->rot.x);
    JSON_ObjectAppendInt(arm_obj, "y_rot", arm->rot.y);
    JSON_ObjectAppendInt(arm_obj, "z_rot", arm->rot.z);
    JSON_ObjectAppendInt(arm_obj, "flash_gun", arm->flash_gun);
    return arm_obj;
}

static JSON_OBJECT *M_DumpAmmo(AMMO_INFO *ammo)
{
    ASSERT(ammo != nullptr);
    JSON_OBJECT *ammo_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(ammo_obj, "ammo", ammo->ammo);
    JSON_ObjectAppendInt(ammo_obj, "hit", ammo->hit);
    JSON_ObjectAppendInt(ammo_obj, "miss", ammo->miss);
    return ammo_obj;
}

static JSON_OBJECT *M_DumpLOT(LOT_INFO *lot)
{
    ASSERT(lot != nullptr);
    JSON_OBJECT *lot_obj = JSON_ObjectNew();
    // JSON_ObjectAppendInt(lot_obj, "node", lot->node);
    JSON_ObjectAppendInt(lot_obj, "head", lot->head);
    JSON_ObjectAppendInt(lot_obj, "tail", lot->tail);
    JSON_ObjectAppendInt(lot_obj, "search_num", lot->search_num);
    JSON_ObjectAppendInt(lot_obj, "block_mask", lot->block_mask);
    JSON_ObjectAppendInt(lot_obj, "step", lot->step);
    JSON_ObjectAppendInt(lot_obj, "drop", lot->drop);
    JSON_ObjectAppendInt(lot_obj, "fly", lot->fly);
    JSON_ObjectAppendInt(lot_obj, "zone_count", lot->zone_count);
    JSON_ObjectAppendInt(lot_obj, "target_box", lot->target_box);
    JSON_ObjectAppendInt(lot_obj, "required_box", lot->required_box);
    JSON_ObjectAppendInt(lot_obj, "x", lot->target.x);
    JSON_ObjectAppendInt(lot_obj, "y", lot->target.y);
    JSON_ObjectAppendInt(lot_obj, "z", lot->target.z);
    return lot_obj;
}

static JSON_OBJECT *M_DumpLara(LARA_INFO *lara)
{
    ASSERT(lara != nullptr);
    JSON_OBJECT *lara_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(lara_obj, "item_number", lara->item_num);
    JSON_ObjectAppendInt(lara_obj, "gun_status", lara->gun_status);
    JSON_ObjectAppendInt(lara_obj, "gun_type", lara->gun_type);
    JSON_ObjectAppendInt(lara_obj, "request_gun_type", lara->request_gun_type);
    JSON_ObjectAppendInt(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    JSON_ObjectAppendInt(lara_obj, "water_status", lara->water_status);
    JSON_ObjectAppendInt(lara_obj, "pose_count", lara->pose_count);
    JSON_ObjectAppendInt(lara_obj, "hit_frame", lara->hit_frame);
    JSON_ObjectAppendInt(lara_obj, "hit_direction", lara->hit_direction);
    JSON_ObjectAppendInt(lara_obj, "air", lara->air);
    JSON_ObjectAppendInt(lara_obj, "dive_count", lara->dive_timer);
    JSON_ObjectAppendInt(lara_obj, "death_count", lara->death_timer);
    JSON_ObjectAppendInt(lara_obj, "current_active", lara->current_active);

    JSON_ObjectAppendInt(lara_obj, "hit_effect_count", lara->hit_effect_count);
    JSON_ObjectAppendInt(
        lara_obj, "hit_effect",
        lara->hit_effect ? Effect_GetNum(lara->hit_effect) : 0);

    JSON_ObjectAppendInt(lara_obj, "mesh_effects", lara->mesh_effects);
    JSON_ARRAY *lara_meshes_arr = JSON_ArrayNew();
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        JSON_ArrayAppendInt(
            lara_meshes_arr, Object_GetMeshOffset(lara->mesh_ptrs[i]));
    }
    JSON_ObjectAppendArray(lara_obj, "meshes", lara_meshes_arr);

    JSON_ObjectAppendInt(lara_obj, "target_angle1", lara->target_angles[0]);
    JSON_ObjectAppendInt(lara_obj, "target_angle2", lara->target_angles[1]);
    JSON_ObjectAppendInt(lara_obj, "turn_rate", lara->turn_rate);
    JSON_ObjectAppendInt(lara_obj, "move_angle", lara->move_angle);
    JSON_ObjectAppendInt(lara_obj, "head_rot.y", lara->head_rot.y);
    JSON_ObjectAppendInt(lara_obj, "head_rot.x", lara->head_rot.x);
    JSON_ObjectAppendInt(lara_obj, "head_rot.z", lara->head_rot.z);
    JSON_ObjectAppendInt(lara_obj, "torso_rot.y", lara->torso_rot.y);
    JSON_ObjectAppendInt(lara_obj, "torso_rot.x", lara->torso_rot.x);
    JSON_ObjectAppendInt(lara_obj, "torso_rot.z", lara->torso_rot.z);

    JSON_ObjectAppendObject(lara_obj, "left_arm", M_DumpArm(&lara->left_arm));
    JSON_ObjectAppendObject(lara_obj, "right_arm", M_DumpArm(&lara->right_arm));
    JSON_ObjectAppendObject(lara_obj, "pistols", M_DumpAmmo(&lara->pistols));
    JSON_ObjectAppendObject(lara_obj, "magnums", M_DumpAmmo(&lara->magnums));
    JSON_ObjectAppendObject(lara_obj, "uzis", M_DumpAmmo(&lara->uzis));
    JSON_ObjectAppendObject(lara_obj, "shotgun", M_DumpAmmo(&lara->shotgun));
    JSON_ObjectAppendObject(lara_obj, "lot", M_DumpLOT(&lara->lot));

    JSON_ObjectAppendInt(
        lara_obj, "interact_target.item_num", lara->interact_target.item_num);
    JSON_ObjectAppendInt(
        lara_obj, "interact_target.move_count",
        lara->interact_target.move_count);
    JSON_ObjectAppendBool(
        lara_obj, "interact_target.is_moving", lara->interact_target.is_moving);

    return lara_obj;
}

static JSON_OBJECT *M_DumpCurrentMusic(void)
{
    const MUSIC_TRACK_ID current_track = Music_GetCurrentPlayingTrack();
    const bool is_ambient = current_track == Music_GetCurrentLoopedTrack();
    JSON_OBJECT *const current_music_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(current_music_obj, "current_track", current_track);
    JSON_ObjectAppendDouble(
        current_music_obj, "timestamp", Music_GetTimestamp());
    JSON_ObjectAppendBool(current_music_obj, "is_ambient", is_ambient);

    return current_music_obj;
}

static JSON_ARRAY *M_DumpMusicTrackFlags(void)
{
    JSON_ARRAY *music_track_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        JSON_ArrayAppendInt(music_track_arr, Music_GetTrackFlags(i));
    }
    return music_track_arr;
}

char *Savegame_BSON_GetSaveFileName(int32_t slot)
{
    size_t out_size =
        snprintf(nullptr, 0, g_GameFlow.savegame_fmt_bson, slot) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, g_GameFlow.savegame_fmt_bson, slot);
    return out;
}

bool Savegame_BSON_FillInfo(MYFILE *fp, SAVEGAME_INFO *info)
{
    bool ret = false;
    JSON_VALUE *root = M_ParseFromFile(fp, nullptr);
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    if (root_obj) {
        info->counter = JSON_ObjectGetInt(root_obj, "save_counter", -1);
        info->level_num = JSON_ObjectGetInt(root_obj, "level_num", -1);
        const char *level_title =
            JSON_ObjectGetString(root_obj, "level_title", nullptr);
        if (level_title) {
            info->level_title = Memory_DupStr(level_title);
        }
        ret = info->level_num != -1;
    }
    JSON_ValueFree(root);

    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, &header, sizeof(SAVEGAME_BSON_HEADER));
    info->initial_version = header.initial_version;
    info->features.restart = header.initial_version >= VERSION_LEGACY;
    info->features.select_level = header.initial_version >= VERSION_1;

    return ret;
}

bool Savegame_BSON_LoadFromFile(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);

    bool ret = false;

    // Read savegame version
    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, &header, sizeof(SAVEGAME_BSON_HEADER));
    File_Seek(fp, 0, FILE_SEEK_SET);

    JSON_VALUE *root = M_ParseFromFile(fp, nullptr);
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    if (!root_obj) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!M_LoadResumeInfo(
            JSON_ObjectGetArray(root_obj, "current_info"),
            game_info->current)) {
        LOG_WARNING(
            "Failed to load RESUME_INFO current properly. "
            "Checking if save is legacy.");
        // Check for 2.6 and 2.7 legacy start and end info.
        if (!M_LoadDiscontinuedStartInfo(
                JSON_ObjectGetArray(root_obj, "start_info"), game_info)) {
            goto cleanup;
        }
        if (!M_LoadDiscontinuedEndInfo(
                JSON_ObjectGetArray(root_obj, "end_info"), game_info)) {
            goto cleanup;
        }
    }

    if (!M_LoadMisc(
            JSON_ObjectGetObject(root_obj, "misc"), game_info,
            header.version)) {
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

    Savegame_ProcessItemsBeforeLoad();

    if (!M_LoadItems(JSON_ObjectGetArray(root_obj, "items"), header.version)) {
        goto cleanup;
    }

    if (header.version >= VERSION_3) {
        if (!M_LoadEffects(JSON_ObjectGetArray(root_obj, "fx"))) {
            goto cleanup;
        }
    }

    if (!M_LoadLara(
            JSON_ObjectGetObject(root_obj, "lara"), &g_Lara, header.version)) {
        goto cleanup;
    }

    if (header.version >= VERSION_3) {
        if (!M_LoadCurrentMusic(JSON_ObjectGetObject(root_obj, "music"))) {
            goto cleanup;
        }

        if (!M_LoadMusicTrackFlags(
                JSON_ObjectGetArray(root_obj, "music_track_flags"))) {
            goto cleanup;
        }
    }

    ret = true;

cleanup:
    JSON_ValueFree(root);
    return ret;
}

bool Savegame_BSON_LoadOnlyResumeInfo(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);

    bool ret = false;
    JSON_VALUE *root = M_ParseFromFile(fp, nullptr);
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    if (!root_obj) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!M_LoadResumeInfo(
            JSON_ObjectGetArray(root_obj, "current_info"),
            game_info->current)) {
        LOG_WARNING(
            "Failed to load RESUME_INFO current properly. Checking if "
            "save is legacy.");
        // Check for 2.6 and 2.7 legacy start and end info.
        if (!M_LoadDiscontinuedStartInfo(
                JSON_ObjectGetArray(root_obj, "start_info"), game_info)) {
            goto cleanup;
        }
        if (!M_LoadDiscontinuedEndInfo(
                JSON_ObjectGetArray(root_obj, "end_info"), game_info)) {
            goto cleanup;
        }
    }

    ret = true;

cleanup:
    JSON_ValueFree(root);
    return ret;
}

void Savegame_BSON_SaveToFile(MYFILE *fp, GAME_INFO *game_info)
{
    ASSERT(game_info != nullptr);

    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    JSON_OBJECT *root_obj = JSON_ObjectNew();

    JSON_ObjectAppendString(root_obj, "level_title", current_level->title);
    JSON_ObjectAppendInt(root_obj, "save_counter", g_SaveCounter);
    JSON_ObjectAppendInt(root_obj, "level_num", current_level->num);

    JSON_ObjectAppendObject(root_obj, "misc", M_DumpMisc(game_info));
    JSON_ObjectAppendArray(
        root_obj, "current_info", M_DumpResumeInfo(game_info->current));
    JSON_ObjectAppendObject(root_obj, "inventory", M_DumpInventory());
    JSON_ObjectAppendObject(root_obj, "flipmap", M_DumpFlipmaps());
    JSON_ObjectAppendArray(root_obj, "cameras", M_DumpCameras());
    JSON_ObjectAppendArray(root_obj, "items", M_DumpItems());
    JSON_ObjectAppendArray(root_obj, "fx", M_DumpEffects());
    JSON_ObjectAppendObject(root_obj, "lara", M_DumpLara(&g_Lara));
    JSON_ObjectAppendObject(root_obj, "music", M_DumpCurrentMusic());
    JSON_ObjectAppendArray(
        root_obj, "music_track_flags", M_DumpMusicTrackFlags());

    JSON_VALUE *root = JSON_ValueFromObject(root_obj);
    M_SaveRaw(fp, root, SAVEGAME_CURRENT_VERSION);
    JSON_ValueFree(root);
}

bool Savegame_BSON_UpdateDeathCounters(MYFILE *fp, GAME_INFO *game_info)
{
    bool result = false;
    int32_t version;
    JSON_VALUE *const root = M_ParseFromFile(fp, &version);
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
    JSON_ObjectAppendInt(misc_obj, "death_count", game_info->death_count);

    File_Seek(fp, 0, FILE_SEEK_SET);
    M_SaveRaw(fp, root, version);
    result = true;

cleanup:
    JSON_ValueFree(root);
    return result;
}
