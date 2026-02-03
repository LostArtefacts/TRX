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
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/file.h>
#include <trx/game/savegame/file_write_io.h>
#include <trx/game/weather_fx.h>
#include <trx/version.h>

typedef struct {
    int16_t count;
    int16_t id_map[MAX_EFFECTS];
} M_FX_ORDER;

static void M_WriteXYZ32(
    SG_WRITE_IO *const io, const char *const key, const XYZ_32 source)
{
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "x", source.x);
    SGW_WRITE_VALUE(io, "y", source.y);
    SGW_WRITE_VALUE(io, "z", source.z);
    SGW_POP_AND_SET(io, key);
}

static void M_WriteXYZ16(
    SG_WRITE_IO *const io, const char *const key, const XYZ_16 source)
{
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "x", source.x);
    SGW_WRITE_VALUE(io, "y", source.y);
    SGW_WRITE_VALUE(io, "z", source.z);
    SGW_POP_AND_SET(io, key);
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
    SG_WRITE_IO *const io, const ITEM *const item,
    const M_FX_ORDER *const fx_order)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    SGW_WRITE_VALUE(io, "object_id", Object_ToGameID(item->object_id));

    if (obj->save_position) {
        M_WriteXYZ32(io, "pos", item->pos);
        M_WriteXYZ16(io, "rot", item->rot);
        SGW_WRITE_VALUE(io, "room_num", item->room_num);
        SGW_WRITE_VALUE(io, "speed", item->speed);
        SGW_WRITE_VALUE(io, "fall_speed", item->fall_speed);
    }

    if (obj->save_anim) {
        SGW_WRITE_VALUE(io, "current_anim", item->current_anim_state);
        SGW_WRITE_VALUE(io, "goal_anim", item->goal_anim_state);
        SGW_WRITE_VALUE(io, "required_anim", item->required_anim_state);
        SGW_WRITE_VALUE(io, "anim_num", item->anim_num);
        SGW_WRITE_VALUE(io, "frame_num", item->frame_num);
        SGW_WRITE_VALUE(io, "prev_frame_num", item->prev_frame_num);
    }

    if (obj->save_hitpoints) {
        SGW_WRITE_VALUE(io, "hitpoints", item->hit_points);
        SGW_WRITE_VALUE(io, "max_hitpoints", item->max_hit_points);
    }

    if (obj->save_flags) {
        SGW_WRITE_VALUE(io, "flags", item->flags);
        SGW_WRITE_VALUE(io, "status", item->status);
        SGW_WRITE_VALUE(io, "active", item->active);
        SGW_WRITE_VALUE(io, "gravity", item->gravity);
        SGW_WRITE_VALUE(io, "collidable", item->collidable);
        const bool intelligent = obj->intelligent && item->data != nullptr;
        SGW_WRITE_VALUE(io, "intelligent", intelligent);
        SGW_WRITE_VALUE(io, "timer", item->timer);
        SGW_WRITE_VALUE_NZ(io, "ai_bits", item->ai_bits);
        SGW_WRITE_VALUE_NZ(io, "ai_tag", item->ai_tag);
        if (intelligent) {
            const CREATURE *const creature = item->data;
            SGW_WRITE_VALUE(io, "head_rot", creature->head_rotation);
            SGW_WRITE_VALUE(io, "neck_rot", creature->neck_rotation);
            SGW_WRITE_VALUE(io, "max_turn", creature->maximum_turn);
            SGW_WRITE_VALUE(io, "creature_flags", creature->flags);
            SGW_WRITE_VALUE(io, "creature_mood", creature->mood);
            SGW_PUSH_OBJECT(io);
            SGW_WRITE_VALUE(io, "alerted", creature->alerted);
            SGW_WRITE_VALUE(io, "head_left", creature->head_left);
            SGW_WRITE_VALUE(io, "head_right", creature->head_right);
            SGW_WRITE_VALUE(io, "reached_goal", creature->reached_goal);
            SGW_WRITE_VALUE(io, "patrol_2", creature->patrol_2);
            SGW_WRITE_VALUE(io, "hurt_by_lara", creature->hurt_by_lara);
            SGW_WRITE_VALUE(io, "damage_from_lara", creature->damage_from_lara);
            SGW_PUSH_ARRAY(io);
            for (int32_t i = 0; i < 4; i++) {
                SGW_PUSH_VALUE(io, creature->joint_rotation[i]);
                SGW_POP_AND_APPEND(io);
            }
            SGW_POP_AND_SET(io, "joint_rotations");
            SGW_POP_AND_SET(io, "creature");
        }
    }

    SGW_PUSH_ARRAY(io);
    const CARRIED_ITEM *drop_item = item->carried_item;
    while (drop_item != nullptr) {
        SGW_PUSH_OBJECT(io);
        SGW_WRITE_VALUE(io, "object_id", Object_ToGameID(drop_item->object_id));
        M_WriteXYZ32(io, "pos", drop_item->pos);
        SGW_WRITE_VALUE(io, "y_rot", drop_item->rot.y);
        SGW_WRITE_VALUE(io, "room_num", drop_item->room_num);
        SGW_WRITE_VALUE(io, "fall_speed", drop_item->fall_speed);
        SGW_WRITE_VALUE(io, "spawn_num", drop_item->spawn_num);
        SGW_WRITE_VALUE(
            io, "status", (int32_t)Carrier_GetSaveStatus(drop_item));
        SGW_POP_AND_APPEND(io);
        drop_item = drop_item->next_item;
    }
    SGW_POP_AND_SET(io, "carried_items");

    if (obj->priv_size > 0 && obj->priv_save_func != nullptr) {
        SGW_PUSH_OBJECT(io);
        obj->priv_save_func(item, io);
        SGW_POP_AND_SET(io, "priv");
    }
}

static void M_WriteArm(
    SG_WRITE_IO *const io, const char *const key, const LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "anim_num", arm->anim_num);
    SGW_WRITE_VALUE(io, "frame_num", arm->frame_num);
    SGW_WRITE_VALUE(io, "lock", arm->lock);
    SGW_WRITE_VALUE(io, "flash_gun", arm->flash_gun);
    M_WriteXYZ16(io, "rot", arm->rot);
    SGW_POP_AND_SET(io, key);
}

static void M_WriteAmmo(
    SG_WRITE_IO *const io, const char *const key, const AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "ammo", ammo->ammo);
    SGW_POP_AND_SET(io, key);
}

static void M_WriteLOT(SG_WRITE_IO *const io, const LOT_INFO *const lot)
{
    ASSERT(lot != nullptr);
    SGW_WRITE_VALUE(io, "head", lot->head);
    SGW_WRITE_VALUE(io, "tail", lot->tail);
    SGW_WRITE_VALUE(io, "search_num", lot->search_num);
    SGW_WRITE_VALUE(io, "block_mask", lot->setup.block_mask);
    SGW_WRITE_VALUE(io, "step", lot->setup.step);
    SGW_WRITE_VALUE(io, "drop", lot->setup.drop);
    SGW_WRITE_VALUE(io, "fly", lot->setup.fly);
    SGW_WRITE_VALUE(io, "zone_count", lot->zone_count);
    SGW_WRITE_VALUE(io, "target_box", lot->target_box);
    SGW_WRITE_VALUE(io, "required_box", lot->required_box);
    SGW_WRITE_VALUE(io, "x", lot->target.x);
    SGW_WRITE_VALUE(io, "y", lot->target.y);
    SGW_WRITE_VALUE(io, "z", lot->target.z);
}

static void M_WriteResumeInfo(
    SG_WRITE_IO *const io, const RESUME_INFO *const resume)
{
    SGW_WRITE_VALUE(io, "available", resume->flags.available);
    SGW_WRITE_VALUE(io, "level_completed", resume->level_completed);
    SGW_WRITE_VALUE(io, "prev_level", resume->prev_level);

    SGW_WRITE_VALUE(io, "hurt_allies", resume->hurt_allies);

    SGW_WRITE_VALUE(io, "lara_hitpoints", resume->lara_hitpoints);
    SGW_WRITE_VALUE(io, "pistol_ammo", resume->pistol_ammo);
    SGW_WRITE_VALUE(io, "shotgun_ammo", resume->shotgun_ammo);
    SGW_WRITE_VALUE(io, "magnum_ammo", resume->magnum_ammo);
    SGW_WRITE_VALUE(io, "autos_ammo", resume->autos_ammo);
    SGW_WRITE_VALUE(io, "desert_eagle_ammo", resume->desert_eagle_ammo);
    SGW_WRITE_VALUE(io, "uzi_ammo", resume->uzi_ammo);
    SGW_WRITE_VALUE(io, "m16_ammo", resume->m16_ammo);
    SGW_WRITE_VALUE(io, "mp5_ammo", resume->mp5_ammo);
    SGW_WRITE_VALUE(io, "grenade_ammo", resume->grenade_ammo);
    SGW_WRITE_VALUE(io, "rocket_ammo", resume->rocket_ammo);
    SGW_WRITE_VALUE(io, "harpoon_ammo", resume->harpoon_ammo);
    SGW_WRITE_VALUE(io, "num_medis", resume->small_medipacks);
    SGW_WRITE_VALUE(io, "num_big_medis", resume->large_medipacks);
    SGW_WRITE_VALUE(io, "num_flares", resume->flares);
    SGW_WRITE_VALUE(io, "num_scions", resume->num_scions);
    SGW_WRITE_VALUE(io, "num_quest_item_1", resume->num_quest_item_1);
    SGW_WRITE_VALUE(io, "num_quest_item_2", resume->num_quest_item_2);
    SGW_WRITE_VALUE(io, "num_quest_item_3", resume->num_quest_item_3);
    SGW_WRITE_VALUE(io, "num_quest_item_4", resume->num_quest_item_4);
    SGW_WRITE_VALUE(io, "gun_status", resume->gun_status);
    SGW_WRITE_VALUE(io, "gun_type", resume->equipped_gun_type);
    SGW_WRITE_VALUE(io, "holsters_gun_type", resume->holsters_gun_type);
    SGW_WRITE_VALUE(io, "back_gun_type", resume->back_gun_type);

    SGW_WRITE_VALUE(io, "has_pistols", resume->flags.has_pistols);
    SGW_WRITE_VALUE(io, "has_shotgun", resume->flags.has_shotgun);
    SGW_WRITE_VALUE(io, "has_magnums", resume->flags.has_magnums);
    SGW_WRITE_VALUE(io, "has_autos", resume->flags.has_autos);
    SGW_WRITE_VALUE(io, "has_desert_eagle", resume->flags.has_desert_eagle);
    SGW_WRITE_VALUE(io, "has_uzis", resume->flags.has_uzis);
    SGW_WRITE_VALUE(io, "has_m16", resume->flags.has_m16);
    SGW_WRITE_VALUE(io, "has_mp5", resume->flags.has_mp5);
    SGW_WRITE_VALUE(io, "has_grenade", resume->flags.has_grenade);
    SGW_WRITE_VALUE(io, "has_rocket", resume->flags.has_rocket);
    SGW_WRITE_VALUE(io, "has_harpoon", resume->flags.has_harpoon);

    SGW_WRITE_VALUE(io, "costume", resume->flags.costume);
    SGW_WRITE_VALUE(io, "timer", resume->stats.timer);
    SGW_WRITE_VALUE(io, "kills", resume->stats.kill_count);
    SGW_WRITE_VALUE(io, "secrets", resume->stats.secret_flags);
    SGW_WRITE_VALUE(io, "pickups", resume->stats.pickup_count);
    SGW_WRITE_VALUE(io, "ammo_hits", resume->stats.ammo_hits);
    SGW_WRITE_VALUE(io, "ammo_used", resume->stats.ammo_used);
    SGW_WRITE_VALUE(io, "distance_travelled", resume->stats.distance_travelled);
    SGW_WRITE_VALUE(io, "medipacks_used", resume->stats.medipacks_used);
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

void SG_File_DumpFlares(SG_WRITE_IO *const io)
{
    SGW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (!item->active || item->object_id != O_FLARE_ITEM) {
            continue;
        }
        SGW_PUSH_OBJECT(io);
        M_WriteXYZ32(io, "pos", item->pos);
        M_WriteXYZ16(io, "rot", item->rot);
        SGW_WRITE_VALUE(io, "room_num", item->room_num);
        SGW_WRITE_VALUE(io, "speed", item->speed);
        SGW_WRITE_VALUE(io, "fall_speed", item->fall_speed);
        SGW_WRITE_VALUE(io, "age", (int32_t)(intptr_t)item->data);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "flares");
}

void SG_File_DumpEffects(SG_WRITE_IO *const io)
{
    M_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    SGW_PUSH_ARRAY(io);
    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        EFFECT *const effect = Effect_Get(link_num);
        if (Object_ToGameID(effect->object_id) == -1) {
            continue;
        }
        SGW_PUSH_OBJECT(io);
        M_WriteXYZ32(io, "pos", effect->pos);
        M_WriteXYZ16(io, "rot", effect->rot);
        SGW_WRITE_VALUE(io, "room_num", effect->room_num);
        SGW_WRITE_VALUE(io, "object_id", Object_ToGameID(effect->object_id));
        SGW_WRITE_VALUE(io, "speed", effect->speed);
        SGW_WRITE_VALUE(io, "fall_speed", effect->fall_speed);
        // Introduced in TRX 1.2
        SGW_WRITE_VALUE(io, "frame_num", effect->frame_num);
        SGW_WRITE_VALUE(io, "frame_number", effect->frame_num);
        SGW_WRITE_VALUE(io, "counter", effect->counter);
        SGW_WRITE_VALUE(io, "shade", effect->shade);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "fx");
}

void SG_File_DumpInventory(SG_WRITE_IO *const io)
{
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "pickup1", Inv_RequestItem(O_PICKUP_ITEM_1));
    SGW_WRITE_VALUE(io, "pickup2", Inv_RequestItem(O_PICKUP_ITEM_2));
    SGW_WRITE_VALUE(io, "quest1", Inv_RequestItem(O_QUEST_ITEM_1));
    SGW_WRITE_VALUE(io, "quest2", Inv_RequestItem(O_QUEST_ITEM_2));
    SGW_WRITE_VALUE(io, "quest3", Inv_RequestItem(O_QUEST_ITEM_3));
    SGW_WRITE_VALUE(io, "quest4", Inv_RequestItem(O_QUEST_ITEM_4));
    SGW_WRITE_VALUE(io, "puzzle1", Inv_RequestItem(O_PUZZLE_ITEM_1));
    SGW_WRITE_VALUE(io, "puzzle2", Inv_RequestItem(O_PUZZLE_ITEM_2));
    SGW_WRITE_VALUE(io, "puzzle3", Inv_RequestItem(O_PUZZLE_ITEM_3));
    SGW_WRITE_VALUE(io, "puzzle4", Inv_RequestItem(O_PUZZLE_ITEM_4));
    SGW_WRITE_VALUE(io, "key1", Inv_RequestItem(O_KEY_ITEM_1));
    SGW_WRITE_VALUE(io, "key2", Inv_RequestItem(O_KEY_ITEM_2));
    SGW_WRITE_VALUE(io, "key3", Inv_RequestItem(O_KEY_ITEM_3));
    SGW_WRITE_VALUE(io, "key4", Inv_RequestItem(O_KEY_ITEM_4));
    SGW_WRITE_VALUE(io, "leadbar", Inv_RequestItem(O_LEADBAR_ITEM));
    SGW_POP_AND_SET(io, "inventory");
}

void SG_File_DumpFlipmaps(SG_WRITE_IO *const io)
{
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "status", Room_GetFlipStatus());
    SGW_WRITE_VALUE(io, "effect", Room_GetFlipEffect());
    SGW_WRITE_VALUE(io, "timer", Room_GetFlipTimer());
    SGW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        SGW_PUSH_VALUE(io, Room_GetFlipSlotFlags(i) >> 8);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "table");
    SGW_POP_AND_SET(io, "flipmap");
}

void SG_File_DumpCameras(SG_WRITE_IO *const io)
{
    SGW_PUSH_ARRAY(io);
    JSON_ARRAY *const cameras_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        SGW_PUSH_VALUE(io, object->flags);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "cameras");
}

void SG_File_DumpMusic(SG_WRITE_IO *const io)
{
    const int32_t track_flag_count = M_GetMusicTrackFlagsCount();
    SGW_PUSH_OBJECT(io);
    SGW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < track_flag_count; i++) {
        SGW_PUSH_VALUE(io, Music_GetTrackFlags(i));
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "flags");

    const MUSIC_ID current_track = Music_GetCurrentPlayingTrack();
    const MUSIC_ID current_ambient = Music_GetCurrentLoopedTrack();
    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "current_track", current_track);
    SGW_WRITE_VALUE(io, "current_ambient", current_ambient);
    SGW_WRITE_VALUE(io, "timestamp", Music_GetTimestamp());
    SGW_POP_AND_SET(io, "current");
    SGW_POP_AND_SET(io, "music");
}

void SG_File_DumpItems(SG_WRITE_IO *const io)
{
    Savegame_ProcessItemsBeforeSave();
    M_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    SGW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        SGW_PUSH_OBJECT(io);
        M_WriteItem(io, Item_Get(i), &fx_order);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "items");
}

void SG_File_DumpLara(SG_WRITE_IO *const io)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    SGW_PUSH_OBJECT(io);

    // Introduced in TRX 1.2
    SGW_WRITE_VALUE(io, "item_num", lara->item_num);
    SGW_WRITE_VALUE(io, "item_number", lara->item_num);
    SGW_WRITE_VALUE(io, "gun_status", lara->gun_status);
    SGW_WRITE_VALUE(io, "gun_type", lara->gun_type);
    SGW_WRITE_VALUE(io, "request_gun_type", lara->request_gun_type);
    SGW_WRITE_VALUE(io, "last_gun_type", lara->last_gun_type);
    SGW_WRITE_VALUE(
        io, "back_gun_obj_id", Object_ToGameID(lara->back_gun_obj_id));
    SGW_WRITE_VALUE(io, "calc_fall_speed", lara->calc_fall_speed);
    SGW_WRITE_VALUE(io, "water_status", lara->water_status);
    SGW_WRITE_VALUE(io, "climb_status", lara->climb_status);
    SGW_WRITE_VALUE(io, "is_crouched", lara->is_crouched);
    SGW_WRITE_VALUE(io, "keep_crouched", lara->keep_crouched);

    SGW_WRITE_VALUE(io, "pose_count", lara->pose_count);
    SGW_WRITE_VALUE(io, "hit_frame", lara->hit_frame);
    SGW_WRITE_VALUE(io, "hit_direction", lara->hit_direction);
    SGW_WRITE_VALUE(io, "hit_effect_count", lara->hit_effect_count);
    SGW_WRITE_VALUE(
        io, "hit_effect",
        lara->hit_effect ? Effect_GetNum(lara->hit_effect) : 0);

    SGW_WRITE_VALUE(io, "air", lara->air);
    SGW_WRITE_VALUE(io, "sprint_timer", lara->sprint_timer);
    SGW_WRITE_VALUE(io, "exposure_timer", lara->exposure_timer);
    SGW_WRITE_VALUE(io, "poison_timer", lara->poison_timer);
    SGW_WRITE_VALUE(io, "dive_count", lara->dive_timer);
    SGW_WRITE_VALUE(io, "death_count", lara->death_timer);

    SGW_WRITE_VALUE(io, "current_active", lara->current.active);
    SGW_WRITE_VALUE(io, "current_vel_x", lara->current.vel.x);
    SGW_WRITE_VALUE(io, "current_vel_z", lara->current.vel.z);
    SGW_WRITE_VALUE(io, "burn", lara->burn);
    SGW_WRITE_VALUE(io, "electric", lara->electric);
    SGW_WRITE_VALUE(io, "water_surface_dist", lara->water_surface_dist);

    SGW_WRITE_VALUE(io, "flare_age", lara->flare.age);
    SGW_WRITE_VALUE(io, "flare_frame", lara->flare.frame_num);
    SGW_WRITE_VALUE(io, "flare_control_left", lara->flare.control);
    SGW_WRITE_VALUE(io, "extra_anim", lara->extra_anim);
    // Introduced in TRX 1.2
    SGW_WRITE_VALUE(io, "vehicle_item_num", Lara_Vehicle_GetIndex());
    SGW_WRITE_VALUE(io, "vehicle_item_number", Lara_Vehicle_GetIndex());

    SGW_WRITE_VALUE(io, "mesh_effects", lara->mesh_effects);

    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "skin_type", Lara_Skin_GetType());
    SGW_WRITE_VALUE(io, "skin_is_default", Lara_Skin_IsDefaultType());
    SGW_WRITE_VALUE(io, "holsters_visible", Lara_Skin_AreHolstersVisible());
    SGW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        const LARA_SKIN_EQUIPMENT *const equipment = Lara_Skin_GetEquipment(i);
        SGW_PUSH_OBJECT(io);
        SGW_WRITE_VALUE(io, "type", equipment->type);
        SGW_WRITE_VALUE(io, "data", equipment->data);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "equipment");
    SGW_POP_AND_SET(io, "skin");

    SGW_WRITE_VALUE(io, "target_angle1", lara->target_angles[0]);
    SGW_WRITE_VALUE(io, "target_angle2", lara->target_angles[1]);
    SGW_WRITE_VALUE(io, "turn_rate", lara->turn_rate);
    SGW_WRITE_VALUE(io, "move_angle", lara->move_angle);
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
        SGW_PUSH_OBJECT(io);
        const ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        SGW_WRITE_VALUE(io, "obj_id", Object_ToGameID(weapon_item->object_id));
        SGW_WRITE_VALUE(io, "anim_num", weapon_item->anim_num);
        SGW_WRITE_VALUE(io, "frame_num", weapon_item->frame_num);
        SGW_WRITE_VALUE(
            io, "current_anim_state", weapon_item->current_anim_state);
        SGW_WRITE_VALUE(io, "goal_anim_state", weapon_item->goal_anim_state);
        SGW_POP_AND_SET(io, "weapon");
    }

    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "item_num", lara->interact_target.item_num);
    SGW_WRITE_VALUE(io, "move_count", lara->interact_target.move_count);
    SGW_WRITE_VALUE(io, "is_moving", lara->interact_target.is_moving);
    SGW_POP_AND_SET(io, "interact_target");

    SGW_POP_AND_SET(io, "lara");
}

void SG_File_DumpResumeInfoList(SG_WRITE_IO *const io)
{
    const int32_t count = GF_GetLevelTable(GFLT_MAIN)->count;
    SGW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        SGW_PUSH_OBJECT(io);
        M_WriteResumeInfo(io, resume);
        SGW_POP_AND_APPEND(io);
    }
    SGW_POP_AND_SET(io, "resume_info");
}

void SG_File_DumpMisc(SG_WRITE_IO *const io)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    SGW_PUSH_OBJECT(io);
    SGW_WRITE_VALUE(io, "game_version", g_TRXVersion);
    SGW_WRITE_VALUE(io, "bonus_flag", Game_GetBonusFlag());
    SGW_WRITE_VALUE(io, "death_count", resume->stats.death_count);
    SGW_WRITE_VALUE(io, "are_monks_angry", Creature_AreAlliesHostile());
    SGW_WRITE_VALUE(io, "sunset_timer", Output_GetTimeInGame());
    SGW_WRITE_VALUE(io, "weather_type", WeatherFX_GetWeather());
    SGW_POP_AND_SET(io, "misc");

    SGW_WRITE_VALUE(
        io, "level_title", level->title != nullptr ? level->title : "");
    SGW_WRITE_VALUE(io, "save_counter", Savegame_GetCounter());
    SGW_WRITE_VALUE(io, "level_num", level->num);
}
