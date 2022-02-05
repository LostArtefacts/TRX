#include "game/savegame_bson.h"

#include "game/ai/pod.h"
#include "game/control.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/shell.h"
#include "game/traps/movable_block.h"
#include "game/traps/rolling_block.h"
#include "global/vars.h"
#include "inv.h"
#include "json.h"
#include "lara.h"
#include "log.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#define MKTAG(a, b, c, d)                                                      \
    ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define SAVEGAME_BSON_MAGIC MKTAG('T', '1', 'M', 'B')

typedef struct SAVEGAME_BSON_HEADER {
    uint32_t magic;
    uint32_t version;
    int32_t compressed_size;
    int32_t uncompressed_size;
} SAVEGAME_BSON_HEADER;

static struct json_value_s *SaveGame_BSON_ParseFromFile(MYFILE *fp);
static bool SaveGame_BSON_LoadLevels(
    struct json_array_s *levels_arr, GAME_INFO *game_info);
static bool SaveGame_BSON_LoadStats(
    struct json_object_s *stats_obj, GAME_INFO *game_info);
static bool SaveGame_BSON_LoadInventory(struct json_object_s *inv_obj);
static bool SaveGame_BSON_LoadFlipmaps(struct json_object_s *flipmap_obj);
static bool SaveGame_BSON_LoadCameras(struct json_array_s *cameras_arr);
static bool SaveGame_BSON_LoadItems(struct json_array_s *items_arr);
static bool SaveGame_BSON_LoadArm(struct json_object_s *arm_obj, LARA_ARM *arm);
static bool SaveGame_BSON_LoadAmmo(
    struct json_object_s *ammo_obj, AMMO_INFO *ammo);
static bool SaveGame_BSON_LoadLOT(struct json_object_s *lot_obj, LOT_INFO *lot);
static bool SaveGame_BSON_LoadLara(
    struct json_object_s *lara_obj, LARA_INFO *lara);
static struct json_array_s *SaveGame_BSON_DumpLevels(GAME_INFO *game_info);
static struct json_object_s *SaveGame_BSON_DumpStats(GAME_INFO *game_info);
static struct json_object_s *SaveGame_BSON_DumpInventory();
static struct json_object_s *SaveGame_BSON_DumpFlipmaps();
static struct json_array_s *SaveGame_BSON_DumpCameras();
static struct json_array_s *SaveGame_BSON_DumpItems();
static struct json_object_s *SaveGame_BSON_DumpArm(LARA_ARM *arm);
static struct json_object_s *SaveGame_BSON_DumpAmmo(AMMO_INFO *ammo);
static struct json_object_s *SaveGame_BSON_DumpLOT(LOT_INFO *lot);
static struct json_object_s *SaveGame_BSON_DumpLara(LARA_INFO *lara);

static struct json_value_s *SaveGame_BSON_ParseFromBuffer(
    const char *buf, size_t buf_size)
{
    SAVEGAME_BSON_HEADER *header = (SAVEGAME_BSON_HEADER *)buf;
    if (header->magic != SAVEGAME_BSON_MAGIC) {
        LOG_ERROR("Invalid savegame magic");
        return NULL;
    }

    const char *compressed = buf + sizeof(SAVEGAME_BSON_HEADER);
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

static struct json_value_s *SaveGame_BSON_ParseFromFile(MYFILE *fp)
{
    size_t buf_size = File_Size(fp);
    char *buf = Memory_Alloc(buf_size);
    File_Seek(fp, 0, FILE_SEEK_SET);
    File_Read(buf, buf_size, 1, fp);

    struct json_value_s *ret = SaveGame_BSON_ParseFromBuffer(buf, buf_size);
    Memory_FreePointer(&buf);
    return ret;
}

static bool SaveGame_BSON_LoadLevels(
    struct json_array_s *levels_arr, GAME_INFO *game_info)
{
    assert(game_info);
    assert(game_info->start);
    if (!levels_arr) {
        LOG_ERROR("Malformed save: invalid or missing levels array");
        return false;
    }
    if ((signed)levels_arr->length != g_GameFlow.level_count) {
        LOG_ERROR(
            "Malformed save: expected %d levels, got %d",
            g_GameFlow.level_count, levels_arr->length);
        return false;
    }
    for (int i = 0; i < (signed)levels_arr->length; i++) {
        struct json_object_s *level_obj = json_array_get_object(levels_arr, i);
        if (!level_obj) {
            LOG_ERROR("Malformed save: invalid level data");
            return false;
        }
        START_INFO *start = &game_info->start[i];
        start->pistol_ammo = json_object_get_int(level_obj, "pistol_ammo", 0);
        start->magnum_ammo = json_object_get_int(level_obj, "magnum_ammo", 0);
        start->uzi_ammo = json_object_get_int(level_obj, "uzi_ammo", 0);
        start->shotgun_ammo = json_object_get_int(level_obj, "shotgun_ammo", 0);
        start->num_medis = json_object_get_int(level_obj, "num_medis", 0);
        start->num_big_medis =
            json_object_get_int(level_obj, "num_big_medis", 0);
        start->num_scions = json_object_get_int(level_obj, "num_scions", 0);
        start->gun_status = json_object_get_int(level_obj, "gun_status", 0);
        start->gun_type = json_object_get_int(level_obj, "gun_type", 0);
        start->flags.all = json_object_get_int(level_obj, "flags", 0);
    }
    return true;
}

static bool SaveGame_BSON_LoadStats(
    struct json_object_s *stats_obj, GAME_INFO *game_info)
{
    assert(game_info);
    if (!stats_obj) {
        LOG_ERROR("Malformed save: invalid or missing stats info");
        return false;
    }
    game_info->timer = json_object_get_int(stats_obj, "timer", 0);
    game_info->kills = json_object_get_int(stats_obj, "kills", 0);
    game_info->secrets = json_object_get_int(stats_obj, "secrets", 0);
    game_info->pickups = json_object_get_int(stats_obj, "pickups", 0);
    game_info->bonus_flag =
        json_object_get_bool(stats_obj, "bonus_flag", false);
    return true;
}

static bool SaveGame_BSON_LoadInventory(struct json_object_s *inv_obj)
{
    if (!inv_obj) {
        LOG_ERROR("Malformed save: invalid or missing inventory info");
        return false;
    }
    InitialiseLaraInventory(g_CurrentLevel);
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

static bool SaveGame_BSON_LoadFlipmaps(struct json_object_s *flipmap_obj)
{
    if (!flipmap_obj) {
        LOG_ERROR("Malformed save: invalid or missing flipmap info");
        return false;
    }

    if (json_object_get_bool(flipmap_obj, "status", false)) {
        FlipMap();
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

static bool SaveGame_BSON_LoadCameras(struct json_array_s *cameras_arr)
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

static bool SaveGame_BSON_LoadItems(struct json_array_s *items_arr)
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
        if (item->object_number != obj_num) {
            LOG_ERROR(
                "Malformed save: expected object %d, got %d",
                item->object_number, obj_num);
            return false;
        }

        if (obj->control == MovableBlockControl) {
            AlterFloorHeight(item, WALL_L);
        }
        if (obj->control == RollingBlockControl) {
            AlterFloorHeight(item, WALL_L * 2);
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
                ItemNewRoom(i, room_num);
            }

            if (obj->shadow_size) {
                FLOOR_INFO *floor =
                    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
                item->floor =
                    GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
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
                KillItem(i);
                item->status = IS_DEACTIVATED;
            } else {
                if (json_object_get_bool(item_obj, "active", item->active)
                    && !item->active) {
                    AddActiveItem(i);
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
                EnableBaddieAI(i, 1);
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

            item->flags &= 0xFF00;

            if (obj->collision == PuzzleHoleCollision
                && (item->status == IS_DEACTIVATED
                    || item->status == IS_ACTIVE)) {
                item->object_number += O_PUZZLE_DONE1 - O_PUZZLE_HOLE1;
            }

            if (obj->control == PodControl && item->status == IS_DEACTIVATED) {
                item->mesh_bits = 0x1FF;
                item->collidable = 0;
            }

            if (obj->collision == PickUpCollision
                && item->status == IS_DEACTIVATED) {
                RemoveDrawnItem(i);
            }
        }

        if (obj->control == MovableBlockControl
            && item->status == IS_NOT_ACTIVE) {
            AlterFloorHeight(item, -WALL_L);
        }

        if (obj->control == RollingBlockControl
            && item->current_anim_state != RBS_MOVING) {
            AlterFloorHeight(item, -WALL_L * 2);
        }

        if (item->object_number == O_PIERRE && item->hit_points <= 0
            && (item->flags & IF_ONESHOT)) {
            if (Inv_RequestItem(O_SCION_ITEM) == 1) {
                SpawnItem(item, O_MAGNUM_ITEM);
                SpawnItem(item, O_SCION_ITEM2);
                SpawnItem(item, O_KEY_ITEM1);
            }
            g_MusicTrackFlags[MX_PIERRE_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_SKATEKID && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_UZI_ITEM)) {
                SpawnItem(item, O_UZI_ITEM);
            }
        }

        if (item->object_number == O_COWBOY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
                SpawnItem(item, O_MAGNUM_ITEM);
            }
            g_MusicTrackFlags[MX_COWBOY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_BALDY && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
                SpawnItem(item, O_SHOTGUN_ITEM);
            }
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }
    }
    return true;
}

static bool SaveGame_BSON_LoadArm(struct json_object_s *arm_obj, LARA_ARM *arm)
{
    assert(arm);
    if (!arm_obj) {
        LOG_ERROR("Malformed save: invalid or missing arm info");
        return false;
    }

    size_t idx = (size_t)arm->frame_base - (size_t)g_AnimFrames;
    idx = json_object_get_int(arm_obj, "frame_base", idx);
    arm->frame_base = (int16_t *)((size_t)g_AnimFrames + (size_t)idx);

    arm->frame_number =
        json_object_get_int(arm_obj, "frame_num", arm->frame_number);
    arm->lock = json_object_get_int(arm_obj, "lock", arm->lock);
    arm->x_rot = json_object_get_int(arm_obj, "x_rot", arm->x_rot);
    arm->y_rot = json_object_get_int(arm_obj, "y_rot", arm->y_rot);
    arm->z_rot = json_object_get_int(arm_obj, "z_rot", arm->z_rot);
    arm->flash_gun = json_object_get_int(arm_obj, "flash_gun", arm->flash_gun);
    return true;
}

static bool SaveGame_BSON_LoadAmmo(
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

static bool SaveGame_BSON_LoadLOT(struct json_object_s *lot_obj, LOT_INFO *lot)
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

static bool SaveGame_BSON_LoadLara(
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
    lara->dive_count =
        json_object_get_int(lara_obj, "dive_count", lara->dive_count);
    lara->death_count =
        json_object_get_int(lara_obj, "death_count", lara->death_count);
    lara->current_active =
        json_object_get_int(lara_obj, "current_active", lara->current_active);

    lara->spaz_effect_count = json_object_get_int(
        lara_obj, "spaz_effect_count", lara->spaz_effect_count);
    lara->spaz_effect = NULL;

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
        size_t idx = (size_t)lara->mesh_ptrs[i] - (size_t)g_MeshBase;
        idx = json_array_get_int(lara_meshes_arr, i, idx);
        lara->mesh_ptrs[i] = (int16_t *)((size_t)g_MeshBase + (size_t)idx);
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

    if (!SaveGame_BSON_LoadArm(
            json_object_get_object(lara_obj, "left_arm"), &lara->left_arm)) {
        return false;
    }

    if (!SaveGame_BSON_LoadArm(
            json_object_get_object(lara_obj, "right_arm"), &lara->right_arm)) {
        return false;
    }

    if (!SaveGame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "pistols"), &lara->pistols)) {
        return false;
    }

    if (!SaveGame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "magnums"), &lara->magnums)) {
        return false;
    }

    if (!SaveGame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "uzis"), &lara->uzis)) {
        return false;
    }

    if (!SaveGame_BSON_LoadAmmo(
            json_object_get_object(lara_obj, "shotgun"), &lara->shotgun)) {
        return false;
    }

    if (!SaveGame_BSON_LoadLOT(
            json_object_get_object(lara_obj, "lot"), &lara->LOT)) {
        return false;
    }

    return true;
}

static struct json_array_s *SaveGame_BSON_DumpLevels(GAME_INFO *game_info)
{
    struct json_array_s *levels_arr = json_array_new();
    assert(game_info->start);
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        START_INFO *start = &game_info->start[i];
        struct json_object_s *level_obj = json_object_new();
        json_object_append_int(level_obj, "pistol_ammo", start->pistol_ammo);
        json_object_append_int(level_obj, "magnum_ammo", start->magnum_ammo);
        json_object_append_int(level_obj, "uzi_ammo", start->uzi_ammo);
        json_object_append_int(level_obj, "shotgun_ammo", start->shotgun_ammo);
        json_object_append_int(level_obj, "num_medis", start->num_medis);
        json_object_append_int(
            level_obj, "num_big_medis", start->num_big_medis);
        json_object_append_int(level_obj, "num_scions", start->num_scions);
        json_object_append_int(level_obj, "gun_status", start->gun_status);
        json_object_append_int(level_obj, "gun_type", start->gun_type);
        json_object_append_int(level_obj, "flags", start->flags.all);
        json_array_append_object(levels_arr, level_obj);
    }
    return levels_arr;
}

static struct json_object_s *SaveGame_BSON_DumpStats(GAME_INFO *game_info)
{
    assert(game_info);
    // TODO: save this info for every level
    struct json_object_s *stats_obj = json_object_new();
    json_object_append_int(stats_obj, "timer", game_info->timer);
    json_object_append_int(stats_obj, "kills", game_info->kills);
    json_object_append_int(stats_obj, "secrets", game_info->secrets);
    json_object_append_int(stats_obj, "pickups", game_info->pickups);
    json_object_append_bool(stats_obj, "bonus_flag", game_info->bonus_flag);
    return stats_obj;
}

static struct json_object_s *SaveGame_BSON_DumpInventory()
{
    // TODO: save this info for every level
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

static struct json_object_s *SaveGame_BSON_DumpFlipmaps()
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

static struct json_array_s *SaveGame_BSON_DumpCameras()
{
    struct json_array_s *cameras_arr = json_array_new();
    for (int i = 0; i < g_NumberCameras; i++) {
        json_array_append_int(cameras_arr, g_Camera.fixed[i].flags);
    }
    return cameras_arr;
}

static struct json_array_s *SaveGame_BSON_DumpItems()
{
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
        }

        json_array_append_object(items_arr, item_obj);
    }
    return items_arr;
}

static struct json_object_s *SaveGame_BSON_DumpArm(LARA_ARM *arm)
{
    assert(arm);
    struct json_object_s *arm_obj = json_object_new();
    json_object_append_int(
        arm_obj, "frame_base", (size_t)arm->frame_base - (size_t)g_AnimFrames);
    json_object_append_int(arm_obj, "frame_num", arm->frame_number);
    json_object_append_int(arm_obj, "lock", arm->lock);
    json_object_append_int(arm_obj, "x_rot", arm->x_rot);
    json_object_append_int(arm_obj, "y_rot", arm->y_rot);
    json_object_append_int(arm_obj, "z_rot", arm->z_rot);
    json_object_append_int(arm_obj, "flash_gun", arm->flash_gun);
    return arm_obj;
}

static struct json_object_s *SaveGame_BSON_DumpAmmo(AMMO_INFO *ammo)
{
    assert(ammo);
    struct json_object_s *ammo_obj = json_object_new();
    json_object_append_int(ammo_obj, "ammo", ammo->ammo);
    json_object_append_int(ammo_obj, "hit", ammo->hit);
    json_object_append_int(ammo_obj, "miss", ammo->miss);
    return ammo_obj;
}

static struct json_object_s *SaveGame_BSON_DumpLOT(LOT_INFO *lot)
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

static struct json_object_s *SaveGame_BSON_DumpLara(LARA_INFO *lara)
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
    json_object_append_int(lara_obj, "dive_count", lara->dive_count);
    json_object_append_int(lara_obj, "death_count", lara->death_count);
    json_object_append_int(lara_obj, "current_active", lara->current_active);

    json_object_append_int(
        lara_obj, "spaz_effect_count", lara->spaz_effect_count);
    json_object_append_int(
        lara_obj, "spaz_effect",
        lara->spaz_effect ? (size_t)lara->spaz_effect - (size_t)g_Effects : 0);

    json_object_append_int(lara_obj, "mesh_effects", lara->mesh_effects);
    struct json_array_s *lara_meshes_arr = json_array_new();
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        json_array_append_int(
            lara_meshes_arr, (size_t)lara->mesh_ptrs[i] - (size_t)g_MeshBase);
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
        lara_obj, "left_arm", SaveGame_BSON_DumpArm(&lara->left_arm));
    json_object_append_object(
        lara_obj, "right_arm", SaveGame_BSON_DumpArm(&lara->right_arm));
    json_object_append_object(
        lara_obj, "pistols", SaveGame_BSON_DumpAmmo(&lara->pistols));
    json_object_append_object(
        lara_obj, "magnums", SaveGame_BSON_DumpAmmo(&lara->magnums));
    json_object_append_object(
        lara_obj, "uzis", SaveGame_BSON_DumpAmmo(&lara->uzis));
    json_object_append_object(
        lara_obj, "shotgun", SaveGame_BSON_DumpAmmo(&lara->shotgun));
    json_object_append_object(
        lara_obj, "lot", SaveGame_BSON_DumpLOT(&lara->LOT));

    return lara_obj;
}

char *SaveGame_BSON_GetSavePath(int32_t slot)
{
    size_t out_size = snprintf(NULL, 0, g_GameFlow.savegame_fmt_bson, slot) + 1;
    char *out = Memory_Alloc(out_size);
    snprintf(out, out_size, g_GameFlow.savegame_fmt_bson, slot);
    return out;
}

int16_t SaveGame_BSON_GetLevelNumber(MYFILE *fp)
{
    struct json_value_s *root = SaveGame_BSON_ParseFromFile(fp);
    struct json_object_s *root_obj = json_value_as_object(root);
    int level_num = -1;
    if (root_obj) {
        level_num = json_object_get_int(root_obj, "level_num", -1);
    }
    json_value_free(root);
    return level_num;
}

int32_t SaveGame_BSON_GetSaveCounter(MYFILE *fp)
{
    struct json_value_s *root = SaveGame_BSON_ParseFromFile(fp);
    struct json_object_s *root_obj = json_value_as_object(root);
    int save_counter = -1;
    if (root_obj) {
        save_counter = json_object_get_int(root_obj, "save_counter", -1);
    }
    json_value_free(root);
    return save_counter;
}

char *SaveGame_BSON_GetLevelTitle(MYFILE *fp)
{
    struct json_value_s *root = SaveGame_BSON_ParseFromFile(fp);
    struct json_object_s *root_obj = json_value_as_object(root);
    char *level_title = NULL;
    if (root_obj) {
        level_title =
            Memory_Dup(json_object_get_string(root_obj, "level_title", NULL));
    }
    json_value_free(root);
    return level_title;
}

bool SaveGame_BSON_ApplySaveBuffer(GAME_INFO *game_info)
{
    assert(game_info);

    bool ret = false;
    struct json_value_s *root = SaveGame_BSON_ParseFromBuffer(
        game_info->savegame_buffer, game_info->savegame_buffer_size);
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

    if (!SaveGame_BSON_LoadLevels(
            json_object_get_array(root_obj, "levels"), game_info)) {
        goto cleanup;
    }

    if (!SaveGame_BSON_LoadStats(
            json_object_get_object(root_obj, "stats"), game_info)) {
        goto cleanup;
    }

    if (!SaveGame_BSON_LoadInventory(
            json_object_get_object(root_obj, "inventory"))) {
        goto cleanup;
    }

    if (!SaveGame_BSON_LoadFlipmaps(
            json_object_get_object(root_obj, "flipmap"))) {
        goto cleanup;
    }

    if (!SaveGame_BSON_LoadCameras(
            json_object_get_array(root_obj, "cameras"))) {
        goto cleanup;
    }

    if (!SaveGame_BSON_LoadItems(json_object_get_array(root_obj, "items"))) {
        goto cleanup;
    }

    BOX_NODE *node = g_Lara.LOT.node;

    if (!SaveGame_BSON_LoadLara(
            json_object_get_object(root_obj, "lara"), &g_Lara)) {
        goto cleanup;
    }

    g_Lara.LOT.node = node;
    g_Lara.LOT.target_box = NO_BOX;

    ret = true;

cleanup:
    json_value_free(root);
    return ret;
}

void SaveGame_BSON_FillSaveBuffer(GAME_INFO *game_info)
{
    assert(game_info);

    struct json_object_s *root_obj = json_object_new();

    json_object_append_string(
        root_obj, "level_title", g_GameFlow.levels[g_CurrentLevel].level_title);
    json_object_append_int(root_obj, "save_counter", g_SaveCounter);
    json_object_append_int(root_obj, "level_num", g_CurrentLevel);

    json_object_append_array(
        root_obj, "levels", SaveGame_BSON_DumpLevels(game_info));
    json_object_append_object(
        root_obj, "stats", SaveGame_BSON_DumpStats(game_info));
    json_object_append_object(
        root_obj, "inventory", SaveGame_BSON_DumpInventory());
    json_object_append_object(
        root_obj, "flipmap", SaveGame_BSON_DumpFlipmaps());
    json_object_append_array(root_obj, "cameras", SaveGame_BSON_DumpCameras());
    json_object_append_array(root_obj, "items", SaveGame_BSON_DumpItems());
    json_object_append_object(
        root_obj, "lara", SaveGame_BSON_DumpLara(&g_Lara));

    size_t uncompressed_size;
    struct json_value_s *root = json_value_from_object(root_obj);
    char *uncompressed = bson_write(root, &uncompressed_size);
    json_value_free(root);

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
        .version = 0,
        .compressed_size = compressed_size,
        .uncompressed_size = uncompressed_size,
    };

    size_t output_size = sizeof(header) + compressed_size;
    assert(output_size <= MAX_SAVEGAME_BUFFER);
    memcpy(game_info->savegame_buffer, &header, sizeof(header));
    memcpy(
        game_info->savegame_buffer + sizeof(header), compressed,
        compressed_size);
    game_info->savegame_buffer_size = output_size;

    Memory_FreePointer(&compressed);
}
