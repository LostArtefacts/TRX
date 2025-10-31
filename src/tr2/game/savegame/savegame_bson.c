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

typedef struct {
    int16_t count;
    int16_t id_map[MAX_EFFECTS];
} M_FX_ORDER;

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

static JSON_ARRAY *M_DumpCameras(void)
{
    JSON_ARRAY *const cameras_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        JSON_ArrayAppendInt(cameras_arr, object->flags);
    }
    return cameras_arr;
}

static void M_GetFXOrder(M_FX_ORDER *const order)
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

    M_FX_ORDER fx_order;
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

static void M_SaveToFile(MYFILE *const fp, SAVEGAME_INFO *const info)
{
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    SAVEGAME_BSON_WRITE_CONTEXT *const ctx = Savegame_BSON_StartWrite();
    JSON_OBJECT *const root_obj = Savegame_BSON_GetWriteRoot(ctx);

    JSON_ObjectAppendObject(root_obj, "misc", M_DumpMisc());
    JSON_ObjectAppendObject(root_obj, "music", M_DumpMusic());
    JSON_ObjectAppendArray(root_obj, "resume_info", M_DumpResumeInfo());
    Savegame_BSON_DumpInventory(ctx);
    JSON_ObjectAppendObject(root_obj, "flipmap", M_DumpFlipmaps());
    JSON_ObjectAppendArray(root_obj, "cameras", M_DumpCameras());
    JSON_ObjectAppendArray(root_obj, "items", M_DumpItems());
    Savegame_BSON_DumpEffects(ctx);
    JSON_ObjectAppendObject(root_obj, "lara", M_DumpLara());
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
