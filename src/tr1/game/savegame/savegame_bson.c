#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"

#include <libtrx/bson.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/general/lift.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/objects/traps/sliding_pillar.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/savegame/bson.h>
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
#define M_NO_ROOM_LEGACY 255

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
static bool M_FillInfo(MYFILE *fp, SAVEGAME_INFO *savegame_info);
static bool M_LoadFromFile(MYFILE *fp);
static bool M_LoadOnlyResumeInfo(MYFILE *fp);
static void M_SaveToFile(MYFILE *fp, SAVEGAME_INFO *savegame_info);
static bool M_UpdateDeathCounters(
    MYFILE *fp, int32_t level_num, int32_t death_count);

static SAVEGAME_STRATEGY m_Strategy = {
    .allow_load = true,
    .allow_save = true,
    .format = SAVEGAME_FORMAT_BSON,
    .get_save_file_pattern_func = M_GetSaveFilePattern,
    .fill_info_func = M_FillInfo,
    .load_from_file_func = M_LoadFromFile,
    .load_only_resume_info_func = M_LoadOnlyResumeInfo,
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

static bool M_LoadResumeInfo(
    JSON_ARRAY *const resume_arr, const uint16_t header_version)
{
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

        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        resume->lara_hitpoints = JSON_ObjectGetInt(
            resume_obj, "lara_hitpoints",
            g_Config.gameplay.start_lara_hitpoints);
        resume->pistol_ammo = JSON_ObjectGetInt(resume_obj, "pistol_ammo", 0);
        resume->magnum_ammo = JSON_ObjectGetInt(resume_obj, "magnum_ammo", 0);
        resume->uzi_ammo = JSON_ObjectGetInt(resume_obj, "uzi_ammo", 0);
        resume->shotgun_ammo = JSON_ObjectGetInt(resume_obj, "shotgun_ammo", 0);
        resume->small_medipacks = JSON_ObjectGetInt(resume_obj, "num_medis", 0);
        resume->large_medipacks =
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
        resume->flags.has_pistols =
            JSON_ObjectGetBool(resume_obj, "got_pistols", 0);
        resume->flags.has_magnums =
            JSON_ObjectGetBool(resume_obj, "got_magnums", 0);
        resume->flags.has_uzis = JSON_ObjectGetBool(resume_obj, "got_uzis", 0);
        resume->flags.has_shotgun =
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
        resume->stats.all_secrets_mask = JSON_ObjectGetInt(
            resume_obj, "all_secrets_mask", resume->stats.all_secrets_mask);
        resume->stats.max_kill_count = JSON_ObjectGetInt(
            resume_obj, "max_kills", resume->stats.max_kill_count);
        resume->stats.max_pickup_count = JSON_ObjectGetInt(
            resume_obj, "max_pickups", resume->stats.max_pickup_count);
        if (header_version >= VERSION_7) {
            resume->stats.ammo_hits = JSON_ObjectGetInt(
                resume_obj, "ammo_hits", resume->stats.ammo_hits);
            resume->stats.ammo_used = JSON_ObjectGetInt(
                resume_obj, "ammo_used", resume->stats.ammo_used);
            resume->stats.medipacks_used = JSON_ObjectGetDouble(
                resume_obj, "medipacks_used", resume->stats.medipacks_used);
            resume->stats.distance_travelled = JSON_ObjectGetInt(
                resume_obj, "distance_travelled",
                resume->stats.distance_travelled);
        }
    }
    return true;
}

static bool M_LoadDiscontinuedStartInfo(JSON_ARRAY *const start_arr)
{
    // This function solely exists for backward compatibility with 2.6 and 2.7
    // saves.
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
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        RESUME_INFO *const start = Savegame_GetCurrentInfo(level);
        start->lara_hitpoints = JSON_ObjectGetInt(
            start_obj, "lara_hitpoints",
            g_Config.gameplay.start_lara_hitpoints);
        start->pistol_ammo = JSON_ObjectGetInt(start_obj, "pistol_ammo", 0);
        start->magnum_ammo = JSON_ObjectGetInt(start_obj, "magnum_ammo", 0);
        start->uzi_ammo = JSON_ObjectGetInt(start_obj, "uzi_ammo", 0);
        start->shotgun_ammo = JSON_ObjectGetInt(start_obj, "shotgun_ammo", 0);
        start->small_medipacks = JSON_ObjectGetInt(start_obj, "num_medis", 0);
        start->large_medipacks =
            JSON_ObjectGetInt(start_obj, "num_big_medis", 0);
        start->num_scions = JSON_ObjectGetInt(start_obj, "num_scions", 0);
        start->gun_status = JSON_ObjectGetInt(start_obj, "gun_status", 0);
        start->equipped_gun_type =
            JSON_ObjectGetInt(start_obj, "gun_type", LGT_UNARMED);
        start->holsters_gun_type = LGT_UNKNOWN;
        start->back_gun_type = LGT_UNKNOWN;
        start->flags.available = JSON_ObjectGetBool(start_obj, "available", 0);
        start->flags.has_pistols =
            JSON_ObjectGetBool(start_obj, "got_pistols", 0);
        start->flags.has_magnums =
            JSON_ObjectGetBool(start_obj, "got_magnums", 0);
        start->flags.has_uzis = JSON_ObjectGetBool(start_obj, "got_uzis", 0);
        start->flags.has_shotgun =
            JSON_ObjectGetBool(start_obj, "got_shotgun", 0);
        start->flags.costume = JSON_ObjectGetBool(start_obj, "costume", 0);
    }
    return true;
}

static bool M_LoadDiscontinuedEndInfo(JSON_ARRAY *end_arr)
{
    // This function solely exists for backward compatibility with 2.6 and 2.7
    // saves.
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

        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        LEVEL_STATS *const end = &resume->stats;
        end->timer = JSON_ObjectGetInt(end_obj, "timer", end->timer);
        end->secret_flags =
            JSON_ObjectGetInt(end_obj, "secrets", end->secret_flags);
        Stats_UpdateSecrets(end);
        end->kill_count = JSON_ObjectGetInt(end_obj, "kills", end->kill_count);
        end->pickup_count =
            JSON_ObjectGetInt(end_obj, "pickups", end->pickup_count);
        end->max_secret_count =
            JSON_ObjectGetInt(end_obj, "max_secrets", end->max_secret_count);
        end->all_secrets_mask = JSON_ObjectGetInt(
            end_obj, "all_secrets_mask", end->all_secrets_mask);
        end->max_kill_count =
            JSON_ObjectGetInt(end_obj, "max_kills", end->max_kill_count);
        end->max_pickup_count =
            JSON_ObjectGetInt(end_obj, "max_pickups", end->max_pickup_count);
    }
    return true;
}

static bool M_LoadMisc(
    JSON_OBJECT *const misc_obj, const uint16_t header_version)
{
    if (!misc_obj) {
        LOG_ERROR("Malformed save: invalid or missing misc info");
        return false;
    }
    const int32_t bonus_flag = JSON_ObjectGetInt(misc_obj, "bonus_flag", 0);
    Game_SetBonusFlag(bonus_flag);
    if (header_version >= VERSION_4) {
        const GF_LEVEL *const current_level = Game_GetCurrentLevel();
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(current_level);
        resume->stats.death_count =
            JSON_ObjectGetInt(misc_obj, "death_count", -1);
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

    Room_SetFlipEffect(JSON_ObjectGetInt(flipmap_obj, "effect", 0));
    Room_SetFlipTimer(JSON_ObjectGetInt(flipmap_obj, "timer", 0));

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
    for (int32_t i = 0; i < (signed)flipmap_arr->length; i++) {
        Room_SetFlipSlotFlags(i, JSON_ArrayGetInt(flipmap_arr, i, 0) << 8);
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

        const OBJECT_ID obj_id =
            Object_FromGameID(JSON_ObjectGetInt(item_obj, "obj_num", -1));
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

            // Prevent issues with pre-injection saves and Lara's enhanced
            // animation set.
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
        }

        JSON_ARRAY *carried_items =
            JSON_ObjectGetArray(item_obj, "carried_items");
        if (carried_items != nullptr) {
            CARRIED_ITEM *carried_item = item->carried_item;
            for (int j = 0; j < (signed)carried_items->length; j++) {
                if (!carried_item) {
                    LOG_ERROR("Malformed save: carried item mismatch");
                    return false;
                }

                JSON_OBJECT *carried_item_obj =
                    JSON_ArrayGetObject(carried_items, j);

                const int32_t object_id =
                    JSON_ObjectGetInt(carried_item_obj, "object_id", -1);
                if (object_id != -1) {
                    carried_item->object_id = Object_FromGameID(object_id);
                }
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

                if (header_version < VERSION_10
                    && carried_item->room_num == M_NO_ROOM_LEGACY) {
                    carried_item->room_num = NO_ROOM;
                }

                carried_item = carried_item->next_item;
            }

            Carrier_TestItemDrops(i);
        } else if (header_version < VERSION_4) {
            Carrier_TestLegacyDrops(i);
        }

        switch (item->object_id) {
        case O_LIFT: {
            JSON_OBJECT *const data_obj =
                JSON_ObjectGetObject(item_obj, "data");
            if (data_obj == nullptr) {
                LOG_ERROR("Malformed save: missing lift data for item %d", i);
                return false;
            }
            LIFT_INFO *const data = (LIFT_INFO *)item->data;
            data->start_height =
                JSON_ObjectGetInt(data_obj, "start_height", data->start_height);
            data->wait_time =
                JSON_ObjectGetInt(data_obj, "wait_time", data->wait_time);
            if (header_version >= VERSION_12) {
                data->is_moving =
                    JSON_ObjectGetBool(data_obj, "is_moving", data->is_moving);
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

    if ((signed)fx_arr->length >= MAX_EFFECTS) {
        LOG_WARNING(
            "Malformed save: expected a max of %d effect, got %d. effect over "
            "the "
            "maximum will not be created.",
            MAX_EFFECTS - 1, fx_arr->length);
    }

    for (int i = 0; i < (signed)fx_arr->length; i++) {
        JSON_OBJECT *fx_obj = JSON_ArrayGetObject(fx_arr, i);
        if (!fx_obj) {
            LOG_ERROR("Malformed save: invalid effect data");
            return false;
        }

        const int16_t room_num = JSON_ObjectGetInt(fx_obj, "room_number", 0);
        const int16_t effect_num = Effect_Create(room_num);
        if (effect_num == NO_EFFECT) {
            continue;
        }
        EFFECT *effect = Effect_Get(effect_num);
        effect->pos.x = JSON_ObjectGetInt(fx_obj, "x", 0);
        effect->pos.y = JSON_ObjectGetInt(fx_obj, "y", 0);
        effect->pos.z = JSON_ObjectGetInt(fx_obj, "z", 0);
        effect->rot.x = JSON_ObjectGetInt(fx_obj, "x_rot", 0);
        effect->rot.y = JSON_ObjectGetInt(fx_obj, "y_rot", 0);
        effect->rot.z = JSON_ObjectGetInt(fx_obj, "z_rot", 0);
        effect->object_id =
            Object_FromGameID(JSON_ObjectGetInt(fx_obj, "object_number", -1));
        effect->speed = JSON_ObjectGetInt(fx_obj, "speed", 0);
        effect->fall_speed = JSON_ObjectGetInt(fx_obj, "fall_speed", 0);
        effect->frame_num = JSON_ObjectGetInt(fx_obj, "frame_number", 0);
        effect->counter = JSON_ObjectGetInt(fx_obj, "counter", 0);
        effect->shade = JSON_ObjectGetInt(fx_obj, "shade", 0);
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
    lot->setup.block_mask =
        JSON_ObjectGetInt(lot_obj, "block_mask", lot->setup.block_mask);
    lot->setup.step = JSON_ObjectGetInt(lot_obj, "step", lot->setup.step);
    lot->setup.drop = JSON_ObjectGetInt(lot_obj, "drop", lot->setup.drop);
    lot->setup.fly = JSON_ObjectGetInt(lot_obj, "fly", lot->setup.fly);
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
    lara->last_gun_type =
        JSON_ObjectGetInt(lara_obj, "last_gun_type", lara->request_gun_type);
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
    lara->burn = JSON_ObjectGetBool(lara_obj, "burn", lara->burn);
    lara->climb_status =
        JSON_ObjectGetInt(lara_obj, "climb_status", lara->climb_status);

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
        } else {
            LOG_WARNING("can't find mesh %d", idx);
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

    if (header_version >= VERSION_7) {
        lara->last_pos.x =
            JSON_ObjectGetInt(lara_obj, "last_pos.x", lara->last_pos.x);
        lara->last_pos.y =
            JSON_ObjectGetInt(lara_obj, "last_pos.y", lara->last_pos.y);
        lara->last_pos.z =
            JSON_ObjectGetInt(lara_obj, "last_pos.z", lara->last_pos.z);
    }

    return true;
}

static bool M_LoadCurrentMusic(
    JSON_OBJECT *music_obj, const uint16_t header_version)
{
    if (music_obj == nullptr) {
        LOG_ERROR("Malformed save: invalid or missing music info");
        return false;
    }

    const MUSIC_ID current_track =
        JSON_ObjectGetInt(music_obj, "current_track", MX_INACTIVE);
    MUSIC_ID ambient_track =
        JSON_ObjectGetInt(music_obj, "current_ambient", MX_INACTIVE);
    const double timestamp = JSON_ObjectGetDouble(music_obj, "timestamp", -1.0);

    if (header_version < VERSION_9) {
        const bool legacy_ambient =
            JSON_ObjectGetBool(music_obj, "is_ambient", false);
        if (legacy_ambient && current_track != MX_INACTIVE) {
            ambient_track = current_track;
        }
    }

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

static bool M_LoadMusicTrackFlags(JSON_ARRAY *music_track_arr)
{
    if (!g_Config.audio.load_music_triggers) {
        return true;
    }

    if (music_track_arr == nullptr) {
        LOG_WARNING("Malformed save: invalid or missing music track array");
        return true;
    }

    if ((signed)music_track_arr->length > MAX_MUSIC_TRACKS) {
        LOG_WARNING(
            "Malformed save: expected at most %d music track flags, got %d",
            MAX_MUSIC_TRACKS, music_track_arr->length);
        return true;
    }

    for (int32_t i = 0; i < (signed)music_track_arr->length; i++) {
        Music_SetTrackFlags(i, JSON_ArrayGetInt(music_track_arr, i, 0));
    }

    return true;
}

static JSON_ARRAY *M_DumpResumeInfo(void)
{
    JSON_ARRAY *resume_arr = JSON_ArrayNew();
    for (int i = 0; i < GF_GetLevelTable(GFLT_MAIN)->count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        JSON_OBJECT *resume_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(
            resume_obj, "lara_hitpoints", resume->lara_hitpoints);
        JSON_ObjectAppendInt(resume_obj, "pistol_ammo", resume->pistol_ammo);
        JSON_ObjectAppendInt(resume_obj, "magnum_ammo", resume->magnum_ammo);
        JSON_ObjectAppendInt(resume_obj, "uzi_ammo", resume->uzi_ammo);
        JSON_ObjectAppendInt(resume_obj, "shotgun_ammo", resume->shotgun_ammo);
        JSON_ObjectAppendInt(resume_obj, "num_medis", resume->small_medipacks);
        JSON_ObjectAppendInt(
            resume_obj, "num_big_medis", resume->large_medipacks);
        JSON_ObjectAppendInt(resume_obj, "num_scions", resume->num_scions);
        JSON_ObjectAppendInt(resume_obj, "gun_status", resume->gun_status);
        JSON_ObjectAppendInt(resume_obj, "gun_type", resume->equipped_gun_type);
        JSON_ObjectAppendInt(
            resume_obj, "holsters_gun_type", resume->holsters_gun_type);
        JSON_ObjectAppendInt(
            resume_obj, "back_gun_type", resume->back_gun_type);
        JSON_ObjectAppendBool(resume_obj, "available", resume->flags.available);
        JSON_ObjectAppendBool(
            resume_obj, "got_pistols", resume->flags.has_pistols);
        JSON_ObjectAppendBool(
            resume_obj, "got_magnums", resume->flags.has_magnums);
        JSON_ObjectAppendBool(resume_obj, "got_uzis", resume->flags.has_uzis);
        JSON_ObjectAppendBool(
            resume_obj, "got_shotgun", resume->flags.has_shotgun);
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
        JSON_ObjectAppendInt(resume_obj, "ammo_hits", resume->stats.ammo_hits);
        JSON_ObjectAppendInt(resume_obj, "ammo_used", resume->stats.ammo_used);
        JSON_ObjectAppendDouble(
            resume_obj, "medipacks_used", resume->stats.medipacks_used);
        JSON_ObjectAppendInt(
            resume_obj, "distance_travelled", resume->stats.distance_travelled);
    }
    return resume_arr;
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
    JSON_ObjectAppendInt(flipmap_obj, "effect", Room_GetFlipEffect());
    JSON_ObjectAppendInt(flipmap_obj, "timer", Room_GetFlipTimer());
    JSON_ARRAY *flipmap_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        JSON_ArrayAppendInt(flipmap_arr, Room_GetFlipSlotFlags(i) >> 8);
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

        JSON_ObjectAppendInt(
            item_obj, "obj_num", Object_ToGameID(item->object_id));

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

        JSON_ARRAY *carried_items_arr = JSON_ArrayNew();

        const CARRIED_ITEM *drop_item = item->carried_item;
        while (drop_item) {
            JSON_OBJECT *drop_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(
                drop_obj, "object_id", Object_ToGameID(drop_item->object_id));
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

        switch (item->object_id) {
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
        JSON_ObjectAppendInt(fx_obj, "x_rot", effect->rot.x);
        JSON_ObjectAppendInt(fx_obj, "y_rot", effect->rot.y);
        JSON_ObjectAppendInt(fx_obj, "z_rot", effect->rot.z);
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
    JSON_ObjectAppendInt(lot_obj, "block_mask", lot->setup.block_mask);
    JSON_ObjectAppendInt(lot_obj, "step", lot->setup.step);
    JSON_ObjectAppendInt(lot_obj, "drop", lot->setup.drop);
    JSON_ObjectAppendInt(lot_obj, "fly", lot->setup.fly);
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
    JSON_ObjectAppendInt(lara_obj, "last_gun_type", lara->last_gun_type);
    JSON_ObjectAppendInt(lara_obj, "calc_fall_speed", lara->calc_fall_speed);
    JSON_ObjectAppendInt(lara_obj, "water_status", lara->water_status);
    JSON_ObjectAppendInt(lara_obj, "pose_count", lara->pose_count);
    JSON_ObjectAppendInt(lara_obj, "hit_frame", lara->hit_frame);
    JSON_ObjectAppendInt(lara_obj, "hit_direction", lara->hit_direction);
    JSON_ObjectAppendInt(lara_obj, "air", lara->air);
    JSON_ObjectAppendInt(lara_obj, "sprint_timer", lara->sprint_timer);
    JSON_ObjectAppendInt(lara_obj, "exposure_timer", lara->exposure_timer);
    JSON_ObjectAppendInt(lara_obj, "dive_count", lara->dive_timer);
    JSON_ObjectAppendInt(lara_obj, "death_count", lara->death_timer);
    JSON_ObjectAppendInt(lara_obj, "current_active", lara->current_active);
    JSON_ObjectAppendBool(lara_obj, "burn", lara->burn);
    JSON_ObjectAppendInt(lara_obj, "climb_status", lara->climb_status);

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
    JSON_ObjectAppendObject(
        lara_obj, "pistols", M_DumpAmmo(&lara->pistol_ammo));
    JSON_ObjectAppendObject(
        lara_obj, "magnums", M_DumpAmmo(&lara->magnum_ammo));
    JSON_ObjectAppendObject(lara_obj, "uzis", M_DumpAmmo(&lara->uzi_ammo));
    JSON_ObjectAppendObject(
        lara_obj, "shotgun", M_DumpAmmo(&lara->shotgun_ammo));
    JSON_ObjectAppendObject(lara_obj, "lot", M_DumpLOT(&lara->lot));

    JSON_ObjectAppendInt(
        lara_obj, "interact_target.item_num", lara->interact_target.item_num);
    JSON_ObjectAppendInt(
        lara_obj, "interact_target.move_count",
        lara->interact_target.move_count);
    JSON_ObjectAppendBool(
        lara_obj, "interact_target.is_moving", lara->interact_target.is_moving);

    JSON_ObjectAppendInt(lara_obj, "last_pos.x", lara->last_pos.x);
    JSON_ObjectAppendInt(lara_obj, "last_pos.y", lara->last_pos.y);
    JSON_ObjectAppendInt(lara_obj, "last_pos.z", lara->last_pos.z);

    return lara_obj;
}

static JSON_OBJECT *M_DumpCurrentMusic(void)
{
    const MUSIC_ID current_track = Music_GetCurrentPlayingTrack();
    const MUSIC_ID current_ambient = Music_GetCurrentLoopedTrack();
    JSON_OBJECT *const current_music_obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(current_music_obj, "current_track", current_track);
    JSON_ObjectAppendInt(current_music_obj, "current_ambient", current_ambient);
    JSON_ObjectAppendDouble(
        current_music_obj, "timestamp", Music_GetTimestamp());

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

static const char *M_GetSaveFilePattern(void)
{
    return g_GameFlow.savegame_fmt_bson;
}

static bool M_FillInfo(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    bool ret = false;
    SAVEGAME_BSON_HEADER header;
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_ReadData(fp, &header, sizeof(SAVEGAME_BSON_HEADER));
    info->initial_version = header.initial_version;
    info->features.restart = header.initial_version >= VERSION_LEGACY;
    info->features.select_level = header.initial_version >= VERSION_1;

    if (header.version >= VERSION_7) {
        // recover the slot information from the end of the file
        File_Skip(fp, header.compressed_size);
        SAVEGAME_BSON_EXTENDED_HEADER extra_header;
        File_ReadData(fp, &extra_header, sizeof(extra_header));
        info->counter = extra_header.counter;
        info->level_num = extra_header.level_num;
        info->level_title = Memory_Alloc(extra_header.title_size + 1);
        File_ReadData(fp, info->level_title, extra_header.title_size);
        ret = true;
    } else {
        // recover the slot information from the bson structures
        File_Seek(fp, 0, FILE_SEEK_SET);
        JSON_VALUE *root = M_ParseFromFile(fp, nullptr);
        JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
        if (root_obj != nullptr) {
            info->counter = JSON_ObjectGetInt(root_obj, "save_counter", -1);
            info->level_num = JSON_ObjectGetInt(root_obj, "level_num", -1);
            const char *level_title =
                JSON_ObjectGetString(root_obj, "level_title", nullptr);
            if (level_title != nullptr) {
                info->level_title = Memory_DupStr(level_title);
            }
            ret = info->level_num != -1;
        }
        JSON_ValueFree(root);
    }

    return ret;
}

static bool M_LoadFromFile(MYFILE *const fp)
{
    bool ret = false;

    int32_t version;
    JSON_VALUE *root = M_ParseFromFile(fp, &version);
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    if (!root_obj) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!M_LoadResumeInfo(
            JSON_ObjectGetArray(root_obj, "current_info"), version)) {
        LOG_WARNING(
            "Failed to load RESUME_INFO current properly. "
            "Checking if save is legacy.");
        // Check for 2.6 and 2.7 legacy start and end info.
        if (!M_LoadDiscontinuedStartInfo(
                JSON_ObjectGetArray(root_obj, "start_info"))) {
            goto cleanup;
        }
        if (!M_LoadDiscontinuedEndInfo(
                JSON_ObjectGetArray(root_obj, "end_info"))) {
            goto cleanup;
        }
    }

    if (!M_LoadMisc(JSON_ObjectGetObject(root_obj, "misc"), version)) {
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

    if (!M_LoadItems(JSON_ObjectGetArray(root_obj, "items"), version)) {
        goto cleanup;
    }

    if (version >= VERSION_3) {
        if (!M_LoadEffects(JSON_ObjectGetArray(root_obj, "fx"))) {
            goto cleanup;
        }
    }

    if (!M_LoadLara(
            JSON_ObjectGetObject(root_obj, "lara"), Lara_GetLaraInfo(),
            version)) {
        goto cleanup;
    }

    if (version >= VERSION_3) {
        if (!M_LoadCurrentMusic(
                JSON_ObjectGetObject(root_obj, "music"), version)) {
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

static bool M_LoadOnlyResumeInfo(MYFILE *const fp)
{
    bool ret = false;

    int32_t version;
    JSON_VALUE *root = M_ParseFromFile(fp, &version);
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    if (!root_obj) {
        LOG_ERROR("Malformed save: cannot parse BSON data");
        goto cleanup;
    }

    if (!M_LoadResumeInfo(
            JSON_ObjectGetArray(root_obj, "current_info"), version)) {
        LOG_WARNING(
            "Failed to load RESUME_INFO current properly. Checking if "
            "save is legacy.");
        // Check for 2.6 and 2.7 legacy start and end info.
        if (!M_LoadDiscontinuedStartInfo(
                JSON_ObjectGetArray(root_obj, "start_info"))) {
            goto cleanup;
        }
        if (!M_LoadDiscontinuedEndInfo(
                JSON_ObjectGetArray(root_obj, "end_info"))) {
            goto cleanup;
        }
    }

    ret = true;

cleanup:
    JSON_ValueFree(root);
    return ret;
}

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const savegame_info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    JSON_OBJECT *root_obj = JSON_ObjectNew();

    JSON_ObjectAppendString(root_obj, "level_title", current_level->title);
    JSON_ObjectAppendInt(root_obj, "save_counter", Savegame_GetCounter());
    JSON_ObjectAppendInt(root_obj, "level_num", current_level->num);

    JSON_ObjectAppendObject(root_obj, "misc", M_DumpMisc());
    JSON_ObjectAppendArray(root_obj, "current_info", M_DumpResumeInfo());
    JSON_ObjectAppendObject(root_obj, "inventory", M_DumpInventory());
    JSON_ObjectAppendObject(root_obj, "flipmap", M_DumpFlipmaps());
    JSON_ObjectAppendArray(root_obj, "cameras", M_DumpCameras());
    JSON_ObjectAppendArray(root_obj, "items", M_DumpItems());
    JSON_ObjectAppendArray(root_obj, "fx", M_DumpEffects());
    JSON_ObjectAppendObject(root_obj, "lara", M_DumpLara(Lara_GetLaraInfo()));
    JSON_ObjectAppendObject(root_obj, "music", M_DumpCurrentMusic());
    JSON_ObjectAppendArray(
        root_obj, "music_track_flags", M_DumpMusicTrackFlags());

    JSON_VALUE *root = JSON_ValueFromObject(root_obj);
    M_SaveRaw(fp, root, SAVEGAME_CURRENT_VERSION, current_level->num);
    JSON_ValueFree(root);

    savegame_info->features.restart = true;
}

static bool M_UpdateDeathCounters(
    MYFILE *const fp, int32_t level_num, const int32_t death_count)
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
    JSON_ObjectAppendInt(misc_obj, "death_count", death_count);

    File_Seek(fp, 0, FILE_SEEK_SET);
    M_SaveRaw(fp, root, version, level_num);
    result = true;

cleanup:
    JSON_ValueFree(root);
    return result;
}

REGISTER_SAVEGAME_STRATEGY(m_Strategy)
