#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/carrier.h>
#include <trx/game/effects.h>
#include <trx/game/game.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/flare_item.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/file.h>
#include <trx/game/weather_fx.h>
#include <trx/json/util/write_io.h>
#include <trx/version.h>

typedef struct {
    int16_t count;
    int16_t id_map[MAX_EFFECTS];
} M_FX_ORDER;

static void M_WriteXYZ32(
    JSON_WRITE_IO *const io, const char *const key, const XYZ_32 source)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "x", source.x);
    JSONW_WRITE_VALUE(io, "y", source.y);
    JSONW_WRITE_VALUE(io, "z", source.z);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteXYZ16(
    JSON_WRITE_IO *const io, const char *const key, const XYZ_16 source)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "x", source.x);
    JSONW_WRITE_VALUE(io, "y", source.y);
    JSONW_WRITE_VALUE(io, "z", source.z);
    JSONW_POP_AND_SET(io, key);
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

static void M_WriteItem(
    JSON_WRITE_IO *const io, const ITEM *const item,
    const M_FX_ORDER *const fx_order)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    JSONW_WRITE_VALUE(io, "object_id", Object_ToGameID(item->object_id));

    if (obj->save_position) {
        M_WriteXYZ32(io, "pos", item->pos);
        M_WriteXYZ16(io, "rot", item->rot);
        JSONW_WRITE_VALUE(io, "room_num", item->room_num);
        JSONW_WRITE_VALUE(io, "speed", item->speed);
        JSONW_WRITE_VALUE(io, "fall_speed", item->fall_speed);
    }

    if (obj->save_anim) {
        JSONW_WRITE_VALUE(io, "current_anim", item->current_anim_state);
        JSONW_WRITE_VALUE(io, "goal_anim", item->goal_anim_state);
        JSONW_WRITE_VALUE(io, "required_anim", item->required_anim_state);
        JSONW_WRITE_VALUE(io, "anim_num", item->anim_num);
        JSONW_WRITE_VALUE(io, "frame_num", item->frame_num);
        JSONW_WRITE_VALUE(io, "prev_frame_num", item->prev_frame_num);
    }

    if (obj->save_hitpoints) {
        JSONW_WRITE_VALUE(io, "hitpoints", item->hit_points);
        JSONW_WRITE_VALUE(io, "max_hitpoints", item->max_hit_points);
    }

    if (obj->save_flags) {
        JSONW_WRITE_VALUE(io, "flags", item->flags);
        JSONW_WRITE_VALUE(io, "status", item->status);
        JSONW_WRITE_VALUE(io, "active", item->active);
        JSONW_WRITE_VALUE(io, "gravity", item->gravity);
        JSONW_WRITE_VALUE(io, "collidable", item->collidable);
        const bool intelligent =
            obj->intelligent && item->creature_data != nullptr;
        JSONW_WRITE_VALUE(io, "intelligent", intelligent);
        JSONW_WRITE_VALUE(io, "timer", item->timer);
        JSONW_WRITE_VALUE_NZ(io, "ai_bits", item->ai_bits);
        JSONW_WRITE_VALUE_NZ(io, "ai_tag", item->ai_tag);
        if (intelligent) {
            const CREATURE *const creature = item->creature_data;
            JSONW_WRITE_VALUE(io, "head_rot", creature->head_rotation);
            JSONW_WRITE_VALUE(io, "neck_rot", creature->neck_rotation);
            JSONW_WRITE_VALUE(io, "max_turn", creature->maximum_turn);
            JSONW_WRITE_VALUE(io, "creature_flags", creature->flags);
            JSONW_WRITE_VALUE(io, "creature_mood", creature->mood);
            JSONW_PUSH_OBJECT(io);
            JSONW_WRITE_VALUE(io, "alerted", creature->alerted);
            JSONW_WRITE_VALUE(io, "head_left", creature->head_left);
            JSONW_WRITE_VALUE(io, "head_right", creature->head_right);
            JSONW_WRITE_VALUE(io, "reached_goal", creature->reached_goal);
            JSONW_WRITE_VALUE(io, "patrol_2", creature->patrol_2);
            JSONW_WRITE_VALUE(io, "hurt_by_lara", creature->hurt_by_lara);
            JSONW_WRITE_VALUE(
                io, "damage_from_lara", creature->damage_from_lara);
            JSONW_PUSH_ARRAY(io);
            for (int32_t i = 0; i < 4; i++) {
                JSONW_PUSH_VALUE(io, creature->joint_rotation[i]);
                JSONW_POP_AND_APPEND(io);
            }
            JSONW_POP_AND_SET(io, "joint_rotations");
            JSONW_POP_AND_SET(io, "creature");
        }
    }

    JSONW_PUSH_ARRAY(io);
    const CARRIED_ITEM *drop_item = item->carried_item;
    while (drop_item != nullptr) {
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE_VALUE(
            io, "object_id", Object_ToGameID(drop_item->object_id));
        M_WriteXYZ32(io, "pos", drop_item->pos);
        JSONW_WRITE_VALUE(io, "y_rot", drop_item->rot.y);
        JSONW_WRITE_VALUE(io, "room_num", drop_item->room_num);
        JSONW_WRITE_VALUE(io, "fall_speed", drop_item->fall_speed);
        JSONW_WRITE_VALUE(io, "spawn_num", drop_item->spawn_num);
        JSONW_WRITE_VALUE(
            io, "status", (int32_t)Carrier_GetSaveStatus(drop_item));
        JSONW_POP_AND_APPEND(io);
        drop_item = drop_item->next_item;
    }
    JSONW_POP_AND_SET(io, "carried_items");

    if (obj->priv_size > 0 && obj->priv_save_func != nullptr) {
        JSONW_PUSH_OBJECT(io);
        obj->priv_save_func(item, io);
        JSONW_POP_AND_SET(io, "priv");
    }
}

static void M_WriteArm(
    JSON_WRITE_IO *const io, const char *const key, const LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "anim_num", arm->anim_num);
    JSONW_WRITE_VALUE(io, "frame_num", arm->frame_num);
    JSONW_WRITE_VALUE(io, "lock", arm->lock);
    JSONW_WRITE_VALUE(io, "flash_gun", arm->flash_gun);
    M_WriteXYZ16(io, "rot", arm->rot);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteAmmo(
    JSON_WRITE_IO *const io, const char *const key, const AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "ammo", ammo->ammo);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteLOT(JSON_WRITE_IO *const io, const LOT_INFO *const lot)
{
    ASSERT(lot != nullptr);
    JSONW_WRITE_VALUE(io, "head", lot->head);
    JSONW_WRITE_VALUE(io, "tail", lot->tail);
    JSONW_WRITE_VALUE(io, "search_num", lot->search_num);
    JSONW_WRITE_VALUE(io, "block_mask", lot->setup.block_mask);
    JSONW_WRITE_VALUE(io, "step", lot->setup.step);
    JSONW_WRITE_VALUE(io, "drop", lot->setup.drop);
    JSONW_WRITE_VALUE(io, "fly", lot->setup.fly);
    JSONW_WRITE_VALUE(io, "zone_count", lot->zone_count);
    JSONW_WRITE_VALUE(io, "target_box", lot->target_box);
    JSONW_WRITE_VALUE(io, "required_box", lot->required_box);
    JSONW_WRITE_VALUE(io, "x", lot->target.x);
    JSONW_WRITE_VALUE(io, "y", lot->target.y);
    JSONW_WRITE_VALUE(io, "z", lot->target.z);
}

static void M_WriteResumeInfo(
    JSON_WRITE_IO *const io, const RESUME_INFO *const resume)
{
    JSONW_WRITE_VALUE(io, "available", resume->flags.available);
    JSONW_WRITE_VALUE(io, "level_completed", resume->level_completed);
    JSONW_WRITE_VALUE(io, "prev_level", resume->prev_level);

    JSONW_WRITE_VALUE(io, "hurt_allies", resume->hurt_allies);

    JSONW_WRITE_VALUE(io, "lara_hitpoints", resume->lara_hitpoints);
    JSONW_WRITE_VALUE(io, "pistol_ammo", resume->pistol_ammo);
    JSONW_WRITE_VALUE(io, "shotgun_ammo", resume->shotgun_ammo);
    JSONW_WRITE_VALUE(io, "magnum_ammo", resume->magnum_ammo);
    JSONW_WRITE_VALUE(io, "autos_ammo", resume->autos_ammo);
    JSONW_WRITE_VALUE(io, "desert_eagle_ammo", resume->desert_eagle_ammo);
    JSONW_WRITE_VALUE(io, "uzi_ammo", resume->uzi_ammo);
    JSONW_WRITE_VALUE(io, "m16_ammo", resume->m16_ammo);
    JSONW_WRITE_VALUE(io, "mp5_ammo", resume->mp5_ammo);
    JSONW_WRITE_VALUE(io, "grenade_ammo", resume->grenade_ammo);
    JSONW_WRITE_VALUE(io, "rocket_ammo", resume->rocket_ammo);
    JSONW_WRITE_VALUE(io, "harpoon_ammo", resume->harpoon_ammo);
    JSONW_WRITE_VALUE(io, "num_medis", resume->small_medipacks);
    JSONW_WRITE_VALUE(io, "num_big_medis", resume->large_medipacks);
    JSONW_WRITE_VALUE(io, "num_flares", resume->flares);
    JSONW_WRITE_VALUE(io, "num_scions", resume->num_scions);
    JSONW_WRITE_VALUE(io, "num_quest_item_1", resume->num_quest_item_1);
    JSONW_WRITE_VALUE(io, "num_quest_item_2", resume->num_quest_item_2);
    JSONW_WRITE_VALUE(io, "num_quest_item_3", resume->num_quest_item_3);
    JSONW_WRITE_VALUE(io, "num_quest_item_4", resume->num_quest_item_4);
    JSONW_WRITE_VALUE(io, "gun_status", resume->gun_status);
    JSONW_WRITE_VALUE(io, "gun_type", resume->equipped_gun_type);
    JSONW_WRITE_VALUE(io, "holsters_gun_type", resume->holsters_gun_type);
    JSONW_WRITE_VALUE(io, "back_gun_type", resume->back_gun_type);

    JSONW_WRITE_VALUE(io, "has_pistols", resume->flags.has_pistols);
    JSONW_WRITE_VALUE(io, "has_shotgun", resume->flags.has_shotgun);
    JSONW_WRITE_VALUE(io, "has_magnums", resume->flags.has_magnums);
    JSONW_WRITE_VALUE(io, "has_autos", resume->flags.has_autos);
    JSONW_WRITE_VALUE(io, "has_desert_eagle", resume->flags.has_desert_eagle);
    JSONW_WRITE_VALUE(io, "has_uzis", resume->flags.has_uzis);
    JSONW_WRITE_VALUE(io, "has_m16", resume->flags.has_m16);
    JSONW_WRITE_VALUE(io, "has_mp5", resume->flags.has_mp5);
    JSONW_WRITE_VALUE(io, "has_grenade", resume->flags.has_grenade);
    JSONW_WRITE_VALUE(io, "has_rocket", resume->flags.has_rocket);
    JSONW_WRITE_VALUE(io, "has_harpoon", resume->flags.has_harpoon);

    JSONW_WRITE_VALUE(io, "costume", resume->flags.costume);
    JSONW_WRITE_VALUE(io, "timer", resume->stats.timer);
    JSONW_WRITE_VALUE(io, "kills", resume->stats.kill_count);
    JSONW_WRITE_VALUE(io, "secrets", resume->stats.secret_flags);
    JSONW_WRITE_VALUE(io, "pickups", resume->stats.pickup_count);
    JSONW_WRITE_VALUE(io, "ammo_hits", resume->stats.ammo_hits);
    JSONW_WRITE_VALUE(io, "ammo_used", resume->stats.ammo_used);
    JSONW_WRITE_VALUE(
        io, "distance_travelled", resume->stats.distance_travelled);
    JSONW_WRITE_VALUE(io, "medipacks_used", resume->stats.medipacks_used);
}

static int32_t M_GetMusicTrackFlagsCount(void)
{
    int32_t last_index = -1;
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        const uint16_t flags = Music_GetTrackFlags(i);
        if (flags != 0) {
            last_index = i;
        }
    }
    return last_index + 1;
}

void SG_File_DumpFlares(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (!item->active || item->object_id != O_FLARE_ITEM) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        M_WriteXYZ32(io, "pos", item->pos);
        M_WriteXYZ16(io, "rot", item->rot);
        JSONW_WRITE_VALUE(io, "room_num", item->room_num);
        JSONW_WRITE_VALUE(io, "speed", item->speed);
        JSONW_WRITE_VALUE(io, "fall_speed", item->fall_speed);
        const int32_t flare_age = FlareItem_GetAge(item);
        const int32_t active = FlareItem_IsActive(item) ? 0x8000 : 0;
        JSONW_WRITE_VALUE(io, "age", flare_age | active);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "flares");
}

void SG_File_DumpEffects(JSON_WRITE_IO *const io)
{
    M_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    JSONW_PUSH_ARRAY(io);
    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        EFFECT *const effect = Effect_Get(link_num);
        if (Object_ToGameID(effect->object_id) == -1) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        M_WriteXYZ32(io, "pos", effect->pos);
        M_WriteXYZ16(io, "rot", effect->rot);
        JSONW_WRITE_VALUE(io, "room_num", effect->room_num);
        JSONW_WRITE_VALUE(io, "object_id", Object_ToGameID(effect->object_id));
        JSONW_WRITE_VALUE(io, "speed", effect->speed);
        JSONW_WRITE_VALUE(io, "fall_speed", effect->fall_speed);
        // Introduced in TRX 1.2
        JSONW_WRITE_VALUE(io, "frame_num", effect->frame_num);
        JSONW_WRITE_VALUE(io, "frame_number", effect->frame_num);
        JSONW_WRITE_VALUE(io, "counter", effect->counter);
        JSONW_WRITE_VALUE(io, "shade", effect->shade);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "fx");
}

void SG_File_DumpInventory(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "pickup1", Inv_RequestItem(O_PICKUP_ITEM_1));
    JSONW_WRITE_VALUE(io, "pickup2", Inv_RequestItem(O_PICKUP_ITEM_2));
    JSONW_WRITE_VALUE(io, "quest1", Inv_RequestItem(O_QUEST_ITEM_1));
    JSONW_WRITE_VALUE(io, "quest2", Inv_RequestItem(O_QUEST_ITEM_2));
    JSONW_WRITE_VALUE(io, "quest3", Inv_RequestItem(O_QUEST_ITEM_3));
    JSONW_WRITE_VALUE(io, "quest4", Inv_RequestItem(O_QUEST_ITEM_4));
    JSONW_WRITE_VALUE(io, "puzzle1", Inv_RequestItem(O_PUZZLE_ITEM_1));
    JSONW_WRITE_VALUE(io, "puzzle2", Inv_RequestItem(O_PUZZLE_ITEM_2));
    JSONW_WRITE_VALUE(io, "puzzle3", Inv_RequestItem(O_PUZZLE_ITEM_3));
    JSONW_WRITE_VALUE(io, "puzzle4", Inv_RequestItem(O_PUZZLE_ITEM_4));
    JSONW_WRITE_VALUE(io, "key1", Inv_RequestItem(O_KEY_ITEM_1));
    JSONW_WRITE_VALUE(io, "key2", Inv_RequestItem(O_KEY_ITEM_2));
    JSONW_WRITE_VALUE(io, "key3", Inv_RequestItem(O_KEY_ITEM_3));
    JSONW_WRITE_VALUE(io, "key4", Inv_RequestItem(O_KEY_ITEM_4));
    JSONW_WRITE_VALUE(io, "leadbar", Inv_RequestItem(O_LEADBAR_ITEM));
    JSONW_POP_AND_SET(io, "inventory");
}

void SG_File_DumpFlipmaps(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "status", Room_GetFlipStatus());
    JSONW_WRITE_VALUE(io, "effect", Room_GetFlipEffect());
    JSONW_WRITE_VALUE(io, "timer", Room_GetFlipTimer());
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        JSONW_PUSH_VALUE(io, Room_GetFlipSlotFlags(i) >> 8);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "table");
    JSONW_POP_AND_SET(io, "flipmap");
}

void SG_File_DumpCameras(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    JSON_ARRAY *const cameras_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        JSONW_PUSH_VALUE(io, object->flags);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "cameras");
}

void SG_File_DumpMusic(JSON_WRITE_IO *const io)
{
    const int32_t track_flag_count = M_GetMusicTrackFlagsCount();
    JSONW_PUSH_OBJECT(io);
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < track_flag_count; i++) {
        JSONW_PUSH_VALUE(io, Music_GetTrackFlags(i));
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "flags");

    const MUSIC_ID current_ambient = Music_GetCurrentLoopedTrack();
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "current_ambient", current_ambient);
    JSONW_PUSH_ARRAY(io);
    const int32_t stream_count = Music_GetStreamCount();
    for (int32_t i = 0; i < stream_count; i++) {
        MUSIC_STREAM_STATE state = {};
        if (!Music_GetStreamState(i, &state)) {
            continue;
        }

        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE_VALUE(io, "track", state.track_id);
        JSONW_WRITE_VALUE(io, "mode", state.mode);
        JSONW_WRITE_VALUE(io, "timestamp", state.timestamp);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "streams");
    JSONW_POP_AND_SET(io, "current");
    JSONW_POP_AND_SET(io, "music");
}

void SG_File_DumpItems(JSON_WRITE_IO *const io)
{
    Savegame_ProcessItemsBeforeSave();
    M_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        JSONW_PUSH_OBJECT(io);
        M_WriteItem(io, Item_Get(i), &fx_order);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "items");
}

void SG_File_DumpLara(JSON_WRITE_IO *const io)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    JSONW_PUSH_OBJECT(io);

    // Introduced in TRX 1.2
    JSONW_WRITE_VALUE(io, "item_num", lara->item_num);
    JSONW_WRITE_VALUE(io, "item_number", lara->item_num);
    JSONW_WRITE_VALUE(io, "gun_status", lara->gun_status);
    JSONW_WRITE_VALUE(io, "gun_type", lara->gun_type);
    JSONW_WRITE_VALUE(io, "request_gun_type", lara->request_gun_type);
    JSONW_WRITE_VALUE(io, "last_gun_type", lara->last_gun_type);

    JSONW_WRITE_VALUE(io, "calc_fall_speed", lara->calc_fall_speed);
    JSONW_WRITE_VALUE(io, "water_status", lara->water_status);
    JSONW_WRITE_VALUE(io, "climb_status", lara->climb_status);
    JSONW_WRITE_VALUE(io, "is_crouched", lara->is_crouched);
    JSONW_WRITE_VALUE(io, "keep_crouched", lara->keep_crouched);

    JSONW_WRITE_VALUE(io, "pose_count", lara->pose_count);
    JSONW_WRITE_VALUE(io, "hit_frame", lara->hit_frame);
    JSONW_WRITE_VALUE(io, "hit_direction", lara->hit_direction);
    JSONW_WRITE_VALUE(io, "hit_effect_count", lara->hit_effect_count);
    JSONW_WRITE_VALUE(
        io, "hit_effect",
        lara->hit_effect ? Effect_GetNum(lara->hit_effect) : 0);

    JSONW_WRITE_VALUE(io, "air", lara->air);
    JSONW_WRITE_VALUE(io, "sprint_timer", lara->sprint_timer);
    JSONW_WRITE_VALUE(io, "exposure_timer", lara->exposure_timer);
    JSONW_WRITE_VALUE(io, "poison_timer", lara->poison_timer);
    JSONW_WRITE_VALUE(io, "dive_count", lara->dive_timer);
    JSONW_WRITE_VALUE(io, "death_count", lara->death_timer);

    JSONW_WRITE_VALUE(io, "current_active", lara->current.active);
    JSONW_WRITE_VALUE(io, "current_vel_x", lara->current.vel.x);
    JSONW_WRITE_VALUE(io, "current_vel_z", lara->current.vel.z);
    JSONW_WRITE_VALUE(io, "burn", lara->burn);
    JSONW_WRITE_VALUE(io, "electric", lara->electric);
    JSONW_WRITE_VALUE(io, "water_surface_dist", lara->water_surface_dist);

    JSONW_WRITE_VALUE(io, "flare_age", lara->flare.age);
    JSONW_WRITE_VALUE(io, "flare_frame", lara->flare.frame_num);
    JSONW_WRITE_VALUE(io, "flare_control_left", lara->flare.control);
    JSONW_WRITE_VALUE(io, "extra_anim", lara->extra_anim);
    // Introduced in TRX 1.2
    JSONW_WRITE_VALUE(io, "vehicle_item_num", Lara_Vehicle_GetIndex());
    JSONW_WRITE_VALUE(io, "vehicle_item_number", Lara_Vehicle_GetIndex());

    JSONW_WRITE_VALUE(io, "mesh_effects", lara->mesh_effects);

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "skin_type", Lara_Skin_GetType());
    JSONW_WRITE_VALUE(io, "skin_is_default", Lara_Skin_IsDefaultType());
    JSONW_WRITE_VALUE(io, "holsters_visible", Lara_Skin_AreHolstersVisible());
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        const LARA_SKIN_EQUIPMENT *const equipment = Lara_Skin_GetEquipment(i);
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE_VALUE(io, "type", equipment->type);
        JSONW_WRITE_VALUE(io, "data", equipment->data);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "equipment");
    JSONW_POP_AND_SET(io, "skin");

    JSONW_WRITE_VALUE(io, "target_angle1", lara->target_angles[0]);
    JSONW_WRITE_VALUE(io, "target_angle2", lara->target_angles[1]);
    JSONW_WRITE_VALUE(io, "turn_rate", lara->turn_rate);
    JSONW_WRITE_VALUE(io, "move_angle", lara->move_angle);
    M_WriteXYZ16(io, "head_rot", lara->head_rot);
    M_WriteXYZ16(io, "torso_rot", lara->torso_rot);
    M_WriteXYZ32(io, "last_pos", lara->last_pos);
    M_WriteArm(io, "left_arm", &lara->left_arm);
    M_WriteArm(io, "right_arm", &lara->right_arm);
    M_WriteAmmo(io, "pistols", &lara->pistol_ammo);
    M_WriteAmmo(io, "shotgun", &lara->shotgun_ammo);
    M_WriteAmmo(io, "magnums", &lara->magnum_ammo);
    M_WriteAmmo(io, "autos", &lara->autos_ammo);
    M_WriteAmmo(io, "desert_eagle", &lara->desert_eagle_ammo);
    M_WriteAmmo(io, "uzis", &lara->uzi_ammo);
    M_WriteAmmo(io, "harpoon", &lara->harpoon_ammo);
    M_WriteAmmo(io, "grenade", &lara->grenade_ammo);
    M_WriteAmmo(io, "rocket", &lara->rocket_ammo);
    M_WriteAmmo(io, "m16", &lara->m16_ammo);
    M_WriteAmmo(io, "mp5", &lara->mp5_ammo);

    if (lara->gun_item_num != NO_ITEM) {
        JSONW_PUSH_OBJECT(io);
        const ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        JSONW_WRITE_VALUE(
            io, "object_id", Object_ToGameID(weapon_item->object_id));
        JSONW_WRITE_VALUE(io, "anim_num", weapon_item->anim_num);
        JSONW_WRITE_VALUE(io, "frame_num", weapon_item->frame_num);
        JSONW_WRITE_VALUE(
            io, "current_anim_state", weapon_item->current_anim_state);
        JSONW_WRITE_VALUE(io, "goal_anim_state", weapon_item->goal_anim_state);
        JSONW_POP_AND_SET(io, "weapon");
    }

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "item_num", lara->interact_target.item_num);
    JSONW_WRITE_VALUE(io, "move_count", lara->interact_target.move_count);
    JSONW_WRITE_VALUE(io, "is_moving", lara->interact_target.is_moving);
    JSONW_POP_AND_SET(io, "interact_target");

    JSONW_POP_AND_SET(io, "lara");
}

void SG_File_DumpResumeInfoList(JSON_WRITE_IO *const io)
{
    const int32_t count = GF_GetLevelTable(GFLT_MAIN)->count;
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        JSONW_PUSH_OBJECT(io);
        M_WriteResumeInfo(io, resume);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "resume_info");
}

void SG_File_DumpMisc(JSON_WRITE_IO *const io)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE_VALUE(io, "game_version", g_TRXVersion);
    JSONW_WRITE_VALUE(io, "bonus_flag", Game_GetBonusFlag());
    JSONW_WRITE_VALUE(io, "death_count", resume->stats.death_count);
    JSONW_WRITE_VALUE(io, "are_monks_angry", Creature_AreAlliesHostile());
    JSONW_WRITE_VALUE(io, "sunset_timer", Output_GetTimeInGame());
    JSONW_WRITE_VALUE(io, "weather_type", WeatherFX_GetWeather());
    JSONW_POP_AND_SET(io, "misc");

    JSONW_WRITE_VALUE(
        io, "level_title", level->title != nullptr ? level->title : "");
    JSONW_WRITE_VALUE(io, "save_counter", Savegame_GetCounter());
    JSONW_WRITE_VALUE(io, "level_num", level->num);
}
