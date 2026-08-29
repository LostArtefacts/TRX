#include <trx/config.h>
#include <trx/core/json/util/value.h>
#include <trx/core/json/util/write_io.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/cutseq.h>
#include <trx/game/effects.h>
#include <trx/game/fx/common.h>
#include <trx/game/fx/weather.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/level/settings.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/flare_item.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/rope.h>
#include <trx/game/rules.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/file.h>
#include <trx/game/savegame/identity.h>
#include <trx/game/waypoint.h>
#include <trx/version.h>

static void M_WriteXYZ32(
    JSON_WRITE_IO *const io, const char *const key, const XYZ_32 source)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "x", source.x);
    JSONW_WRITE(io, "y", source.y);
    JSONW_WRITE(io, "z", source.z);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteXYZ16(
    JSON_WRITE_IO *const io, const char *const key, const XYZ_16 source)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "x", source.x);
    JSONW_WRITE(io, "y", source.y);
    JSONW_WRITE(io, "z", source.z);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteXYZ32Array(
    JSON_WRITE_IO *const io, const char *const key, const XYZ_32 *const source,
    const int32_t count)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < count; i++) {
        JSONW_PUSH_VALUE(io, source[i]);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteRopeState(JSON_WRITE_IO *const io)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ROPE *const rope =
        lara->rope.index != NO_ROPE ? Rope_Get(lara->rope.index) : nullptr;
    if (rope == nullptr) {
        return;
    }

    const ROPE_PENDULUM *const pendulum = Rope_GetPendulum();

    JSONW_PUSH_OBJECT(io);
    M_WriteXYZ32Array(io, "segments", rope->segments, ROPE_SEGMENTS);
    M_WriteXYZ32Array(io, "velocities", rope->velocities, ROPE_SEGMENTS);
    M_WriteXYZ32Array(
        io, "normalised_segments", rope->normalised_segments, ROPE_SEGMENTS);
    M_WriteXYZ32Array(io, "mesh_segments", rope->mesh_segments, ROPE_SEGMENTS);
    M_WriteXYZ32Array(
        io, "prev_mesh_segments", rope->prev_mesh_segments, ROPE_SEGMENTS);
    JSONW_WRITE(io, "pos", rope->pos);
    JSONW_WRITE(io, "segment_length", rope->segment_length);
    JSONW_WRITE(io, "active", rope->active);

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "pos", pendulum->pos);
    JSONW_WRITE(io, "vel", pendulum->vel);
    JSONW_WRITE(io, "node", pendulum->node);
    JSONW_POP_AND_SET(io, "pendulum");

    JSONW_POP_AND_SET(io, "rope_state");
}

// Global anim indices shift as injections append anims, so persist the owning
// object and an index relative to it.
static void M_WriteAnimNum(JSON_WRITE_IO *const io, const int16_t anim_num)
{
    JSONW_WRITE(io, "anim_num", anim_num);
    CATALOG_FOR_EACH(CATALOG_OBJECTS, id)
    {
        const OBJECT *const obj = Object_Get(id);
        if (!obj->loaded || obj->anim_idx == NO_ANIM || anim_num < obj->anim_idx
            || anim_num >= obj->anim_idx + obj->anim_count) {
            continue;
        }
        const int32_t game_id = Object_IDToSlot(id);
        if (game_id != -1) {
            JSONW_WRITE(io, "anim_obj", game_id);
            JSONW_WRITE(io, "anim_rel", anim_num - obj->anim_idx);
        }
        break;
    }
}

// Pack the item lifecycle axes into the released save format's status value.
// The priority order reproduces the old active/status divergences: an
// ambushing item is simulated but hidden (packs to IS_INVISIBLE), a trap
// playing out its finish is simulated but spent (IS_DEACTIVATED). is_simulated
// itself round-trips through the separate "active" field, is_finished through
// "finished" - the enum is mutually exclusive and a hidden finished item packs
// to IS_INVISIBLE, which older readers take as the whole of its state.
static ITEM_STATUS M_PackItemStatus(const ITEM *const item)
{
    if (!item->is_visible) {
        return IS_INVISIBLE;
    }
    if (item->is_finished) {
        return IS_DEACTIVATED;
    }
    if (item->is_simulated) {
        return IS_ACTIVE;
    }
    return IS_INACTIVE;
}

// Encode the runtime trigger fields and the two synthesized axis bits back into
// the released save format's flags word.
static uint16_t M_PackItemFlags(const ITEM *const item)
{
    return ((uint16_t)item->trigger.mask << TRIGGER_MASK_SHIFT)
        | (item->trigger.reversed ? IF_REVERSE : 0)
        | (item->trigger.switch_spent ? IF_ONE_SHOT_SWITCH : 0)
        | (item->trigger.anti_spent ? IF_ONE_SHOT_ANTITRIGGER : 0)
        | (item->trigger.spent ? IF_ONE_SHOT : 0)
        | (item->is_destroyed ? IF_DESTROYED : 0);
}

static void M_WriteItem(JSON_WRITE_IO *const io, const ITEM *const item)
{
    JSONW_WRITE(io, "index", Item_GetIndex(item));
    if (item->name != nullptr) {
        JSONW_WRITE(io, "name", item->name);
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    SaveGame_WriteIdentity(
        io, "object_id", "object_key", CATALOG_OBJECTS, item->object_id);
    JSONW_WRITE(io, "mesh_bits", item->mesh_bits);

    if (obj->save_position) {
        M_WriteXYZ32(io, "pos", item->pos);
        M_WriteXYZ16(io, "rot", item->rot);
        JSONW_WRITE(io, "room_num", item->room_num);
        JSONW_WRITE(io, "speed", item->speed);
        JSONW_WRITE(io, "fall_speed", item->fall_speed);
    }

    if (obj->save_anim) {
        JSONW_WRITE(io, "current_anim", item->current_anim_state);
        JSONW_WRITE(io, "goal_anim", item->goal_anim_state);
        JSONW_WRITE(io, "required_anim", item->required_anim_state);
        M_WriteAnimNum(io, item->anim_num);
        JSONW_WRITE(io, "frame_num", item->frame_num);
        JSONW_WRITE(io, "prev_frame_num", item->prev_frame_num);
    }

    if (obj->save_hitpoints) {
        JSONW_WRITE(io, "hitpoints", item->hit_points);
        JSONW_WRITE(io, "max_hitpoints", item->max_hit_points);
    }
    ObjectProperty_WriteItemOverrides(io, item, "properties");

    if (obj->save_flags) {
        JSONW_WRITE(io, "flags", M_PackItemFlags(item));
        JSONW_WRITE(io, "status", M_PackItemStatus(item));
        JSONW_WRITE(io, "active", item->is_simulated);
        // is_finished also reaches the status value above, but only where the
        // mutually exclusive enum can hold it; this key carries the axis whole.
        JSONW_WRITE(io, "finished", item->is_finished);
        JSONW_WRITE(io, "gravity", item->gravity);
        JSONW_WRITE(io, "collidable", item->is_collidable);
        const bool intelligent =
            obj->intelligent && item->creature_data != nullptr;
        JSONW_WRITE(io, "intelligent", intelligent);
        JSONW_WRITE(io, "timer", item->timer);
        JSONW_WRITE_NZ(io, "ai_bits", item->ai_bits);
        JSONW_WRITE_NZ(io, "ai_tag", item->ai_tag);
        JSONW_WRITE_NZ(io, "fade", item->fade);
        if (intelligent) {
            const CREATURE *const creature = item->creature_data;
            JSONW_WRITE(io, "head_rot", creature->head_rotation);
            JSONW_WRITE(io, "neck_rot", creature->neck_rotation);
            JSONW_WRITE(io, "max_turn", creature->maximum_turn);
            JSONW_WRITE(io, "creature_flags", creature->flags);
            JSONW_WRITE(io, "creature_mood", creature->mood);
            JSONW_PUSH_OBJECT(io);
            JSONW_WRITE(io, "alerted", creature->alerted);
            JSONW_WRITE(io, "head_left", creature->head_left);
            JSONW_WRITE(io, "head_right", creature->head_right);
            JSONW_WRITE(io, "reached_goal", creature->reached_goal);
            JSONW_WRITE(io, "patrol_2", creature->patrol_2);
            JSONW_WRITE(io, "hurt_by_lara", creature->hurt_by_lara);
            JSONW_WRITE(io, "damage_from_lara", creature->damage_from_lara);
            JSONW_WRITE(
                io, "enemy",
                creature->enemy == nullptr ? NO_ITEM
                                           : Item_GetIndex(creature->enemy));
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
        XYZ_32 drop_pos = drop_item->pos;
        int16_t drop_rot_y = drop_item->rot.y;
        int16_t drop_room_num = drop_item->room_num;
        int16_t drop_fall_speed = drop_item->fall_speed;
        const DROP_STATUS save_status = Carrier_GetSaveStatus(drop_item);

        if ((save_status == DS_FALLING || save_status == DS_DROPPED)
            && drop_item->spawn_num != NO_ITEM) {
            const ITEM *const pickup = Item_Get(drop_item->spawn_num);
            if (pickup != nullptr) {
                drop_pos = pickup->pos;
                drop_rot_y = pickup->rot.y;
                drop_room_num = pickup->room_num;
                drop_fall_speed = pickup->fall_speed;
            }
        }

        JSONW_PUSH_OBJECT(io);
        SaveGame_WriteIdentity(
            io, "object_id", "object_key", CATALOG_OBJECTS,
            drop_item->object_id);
        M_WriteXYZ32(io, "pos", drop_pos);
        JSONW_WRITE(io, "y_rot", drop_rot_y);
        JSONW_WRITE(io, "room_num", drop_room_num);
        JSONW_WRITE(io, "fall_speed", drop_fall_speed);
        JSONW_WRITE(io, "spawn_num", drop_item->spawn_num);
        JSONW_WRITE(io, "status", (int32_t)save_status);
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
    M_WriteAnimNum(io, arm->anim_num);
    JSONW_WRITE(io, "frame_num", arm->frame_num);
    JSONW_WRITE(io, "lock", arm->lock);
    JSONW_WRITE(io, "flash_gun", arm->flash_gun);
    M_WriteXYZ16(io, "rot", arm->rot);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteAmmo(
    JSON_WRITE_IO *const io, const char *const key, const int32_t ammo)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "ammo", ammo);
    JSONW_POP_AND_SET(io, key);
}

static void M_WriteLOT(JSON_WRITE_IO *const io, const LOT_INFO *const lot)
{
    ASSERT(lot != nullptr);
    JSONW_WRITE(io, "head", lot->head);
    JSONW_WRITE(io, "tail", lot->tail);
    JSONW_WRITE(io, "search_num", lot->search_num);
    JSONW_WRITE(io, "block_mask", lot->setup.block_mask);
    JSONW_WRITE(io, "step", lot->setup.step);
    JSONW_WRITE(io, "drop", lot->setup.drop);
    JSONW_WRITE(io, "fly", lot->setup.fly);
    JSONW_WRITE(io, "zone_count", lot->zone_count);
    JSONW_WRITE(io, "target_box", lot->target_box);
    JSONW_WRITE(io, "required_box", lot->required_box);
    JSONW_WRITE(io, "x", lot->target.x);
    JSONW_WRITE(io, "y", lot->target.y);
    JSONW_WRITE(io, "z", lot->target.z);
}

static void M_WriteResumeInfo(
    JSON_WRITE_IO *const io, const RESUME_INFO *const resume)
{
    JSONW_WRITE(io, "available", resume->flags.available);
    JSONW_WRITE(io, "level_completed", resume->level_completed);
    JSONW_WRITE(io, "prev_level", resume->prev_level);

    JSONW_WRITE(io, "hurt_allies", resume->hurt_allies);
    JSONW_WRITE(io, "burning", resume->burning);

    JSONW_WRITE(io, "lara_hitpoints", resume->lara_hitpoints);
    JSONW_WRITE(io, "gun_status", resume->gun_status);
    JSONW_WRITE(io, "gun_type", resume->equipped_gun_type);
    JSONW_WRITE(io, "holsters_gun_type", resume->holsters_gun_type);
    JSONW_WRITE(io, "back_gun_type", resume->back_gun_type);

    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        if (info->save_resume_has_key == nullptr) {
            continue;
        }
        JSONW_WRITE(
            io, info->save_resume_has_key,
            Inv_State_Has(&resume->inv, Gun_GetGunObject(info->gun_type)));
        JSONW_WRITE(
            io, info->save_resume_ammo_key,
            Inv_State_GetAmmo(&resume->inv, info->gun_type));
    }
    JSONW_WRITE(
        io, "has_binoculars", Inv_State_Has(&resume->inv, O_BINOCULARS_ITEM));

    for (const SAVEGAME_RESUME_ITEM *entry = g_Savegame_ResumeItems;
         entry->key != nullptr; entry++) {
        JSONW_WRITE(
            io, entry->key, Inv_State_GetCount(&resume->inv, entry->object_id));
    }

    JSONW_WRITE(io, "costume", resume->flags.costume);
    JSONW_WRITE(io, "timer", resume->stats.timer);
    JSONW_WRITE(io, "kills", resume->stats.counts[STATS_CAT_KILLS]);
    JSONW_WRITE(io, "secrets", resume->stats.secret_flags);
    JSONW_WRITE(io, "crystals", resume->stats.counts[STATS_CAT_CRYSTALS]);
    JSONW_WRITE(io, "pickups", resume->stats.counts[STATS_CAT_PICKUPS]);
    JSONW_WRITE(io, "ammo_hits", resume->stats.ammo_hits);
    JSONW_WRITE(io, "ammo_used", resume->stats.ammo_used);
    JSONW_WRITE(io, "distance_travelled", resume->stats.distance_travelled);
    JSONW_WRITE(io, "medipacks_used", resume->stats.medipacks_used);
    JSONW_WRITE(io, "death_count", resume->stats.death_count);
}

static uint16_t M_PackMusicTrackFlags(const MUSIC_SLOT track_id)
{
    const MUSIC_TRACK_STATE *const track = Music_GetTrackState(track_id);
    return (track->mask << TRIGGER_MASK_SHIFT)
        | (track->is_one_shot ? MTF_ONE_SHOT : 0) | track->delay;
}

static int32_t M_GetMusicTrackFlagsCount(void)
{
    int32_t last_index = -1;
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        if (M_PackMusicTrackFlags(i) != 0) {
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
        if (!item->is_simulated || item->object_id != O_FLARE_ITEM) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        M_WriteXYZ32(io, "pos", item->pos);
        M_WriteXYZ16(io, "rot", item->rot);
        JSONW_WRITE(io, "room_num", item->room_num);
        JSONW_WRITE(io, "speed", item->speed);
        JSONW_WRITE(io, "fall_speed", item->fall_speed);
        const int32_t flare_age = FlareItem_GetAge(item);
        const int32_t active = FlareItem_IsActive(item) ? 0x8000 : 0;
        JSONW_WRITE(io, "age", flare_age | active);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "flares");
}

void SG_File_DumpEffects(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        EFFECT *const effect = Effect_Get(link_num);
        if (Object_IDToSlot(effect->object_id) == -1) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        M_WriteXYZ32(io, "pos", effect->pos);
        M_WriteXYZ16(io, "rot", effect->rot);
        JSONW_WRITE(io, "room_num", effect->room_num);
        SaveGame_WriteIdentity(
            io, "object_id", "object_key", CATALOG_OBJECTS, effect->object_id);
        JSONW_WRITE(io, "speed", effect->speed);
        JSONW_WRITE(io, "fall_speed", effect->fall_speed);
        // Introduced in TRX 1.2
        JSONW_WRITE(io, "frame_num", effect->frame_num);
        JSONW_WRITE(io, "frame_number", effect->frame_num);
        JSONW_WRITE(io, "counter", effect->counter);
        JSONW_WRITE(io, "shade", effect->shade);
        JSONW_WRITE(io, "flag1", effect->flag1);
        JSONW_WRITE(io, "flag2", effect->flag2);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "effects");
}

void SG_File_DumpFX(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    FX_Save(io);
    JSONW_POP_AND_SET_NZ(io, "vfx");
}

void SG_File_DumpInventory(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    for (const SAVEGAME_INVENTORY_ENTRY *entry = g_Savegame_InventoryItems;
         entry->object_id != NO_OBJECT; entry++) {
        JSONW_WRITE(io, entry->key, Inv_GetItemCount(entry->object_id));
    }
    JSONW_POP_AND_SET(io, "inventory");
}

void SG_File_DumpFlipmaps(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "status", Room_GetFlipStatus());
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        JSONW_PUSH_VALUE(io, Room_GetFlipGroupStatus(i));
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "groups");
    JSONW_WRITE(io, "effect", Room_GetFlipEffect());
    JSONW_WRITE(io, "timer", Room_GetFlipTimer());
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        const FLIP_SLOT *const slot = Room_GetFlipSlot(i);
        const uint16_t flags = (slot->mask << TRIGGER_MASK_SHIFT)
            | (slot->is_one_shot ? FSF_ONE_SHOT : 0);
        JSONW_PUSH_VALUE(io, flags >> 8);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "table");
    JSONW_POP_AND_SET(io, "flipmap");
}

void SG_File_DumpCameras(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        JSONW_PUSH_VALUE(io, object->flags);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "cameras");

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Camera_GetSequenceCount(); i++) {
        const FLYBY_SEQUENCE *const sequence = Camera_GetSequence(i);
        JSONW_PUSH_VALUE(io, sequence->one_shot);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "flyby_sequences");
}

void SG_File_DumpMusic(JSON_WRITE_IO *const io)
{
    const int32_t track_flag_count = M_GetMusicTrackFlagsCount();
    JSONW_PUSH_OBJECT(io);
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < track_flag_count; i++) {
        JSONW_PUSH_VALUE(io, M_PackMusicTrackFlags(i));
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "flags");

    // Write both music flag layouts. Positional flags keep older readers
    // working; named flags keep the data tied to catalog identities.
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < track_flag_count; i++) {
        const uint16_t flags = M_PackMusicTrackFlags(i);
        if (flags == 0) {
            continue;
        }
        const CATALOG_ID id = Music_SlotToID(i);
        JSONW_PUSH_OBJECT(io);
        SaveGame_WriteIdentity(io, "slot", "key", CATALOG_MUSIC, id);
        JSONW_WRITE(io, "flags", flags);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "track_flags");

    const MUSIC_SLOT current_ambient = Music_GetCurrentLoopedTrack();
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "current_ambient", current_ambient);
    JSONW_PUSH_ARRAY(io);
    const int32_t stream_count = Music_GetStreamCount();
    for (int32_t i = 0; i < stream_count; i++) {
        MUSIC_STREAM_STATE state = {};
        if (!Music_GetStreamState(i, &state)) {
            continue;
        }

        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "track", state.track_id);
        JSONW_WRITE(io, "mode", state.mode);
        JSONW_WRITE(io, "timestamp", state.timestamp);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "streams");
    JSONW_POP_AND_SET(io, "current");
    JSONW_POP_AND_SET(io, "music");
}

void SG_File_DumpItems(JSON_WRITE_IO *const io)
{
    Savegame_ProcessItemsBeforeSave();

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        JSONW_PUSH_OBJECT(io);
        M_WriteItem(io, Item_Get(i));
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
    JSONW_WRITE(io, "item_num", lara->item_num);
    JSONW_WRITE(io, "item_number", lara->item_num);
    JSONW_WRITE(io, "gun_status", lara->gun_status);
    JSONW_WRITE(io, "gun_type", lara->gun_type);
    JSONW_WRITE(io, "request_gun_type", lara->request_gun_type);
    JSONW_WRITE(io, "last_gun_type", lara->last_gun_type);

    JSONW_WRITE(io, "calc_fall_speed", lara->calc_fall_speed);
    JSONW_WRITE(io, "water_status", lara->water_status);
    JSONW_WRITE(io, "climb_status", lara->climb_status);
    JSONW_WRITE(io, "corner_pos_x", lara->corner_pos.x);
    JSONW_WRITE(io, "corner_pos_z", lara->corner_pos.z);
    JSONW_WRITE(io, "is_crouched", lara->is_crouched);
    JSONW_WRITE(io, "keep_crouched", lara->keep_crouched);
    JSONW_WRITE(io, "sprinting", lara->sprinting);

    JSONW_WRITE(io, "pose_count", lara->pose_count);
    JSONW_WRITE(io, "hit_frame", lara->hit_frame);
    JSONW_WRITE(io, "hit_direction", lara->hit_direction);
    JSONW_WRITE(io, "hit_effect_count", lara->hit_effect_count);
    JSONW_WRITE(
        io, "hit_effect",
        lara->hit_effect ? Effect_GetIndex(lara->hit_effect) : 0);

    JSONW_WRITE(io, "air", lara->air);
    JSONW_WRITE(io, "sprint_timer", lara->sprint_timer);
    JSONW_WRITE(io, "exposure_timer", lara->exposure_timer);
    JSONW_WRITE(io, "poison_timer", lara->poison.value);
    JSONW_WRITE(io, "poison_target", lara->poison.target);
    JSONW_WRITE(io, "dive_count", lara->dive_timer);
    JSONW_WRITE(io, "death_count", lara->death_timer);

    JSONW_WRITE(io, "current_active", lara->current.active);
    JSONW_WRITE(io, "current_vel_x", lara->current.vel.x);
    JSONW_WRITE(io, "current_vel_z", lara->current.vel.z);
    JSONW_WRITE(io, "burn", lara->burn);
    JSONW_WRITE(io, "electric", lara->electric);
    JSONW_WRITE(io, "water_surface_dist", lara->water_surface_dist);

    JSONW_WRITE(io, "flare_age", lara->flare.age);
    JSONW_WRITE(io, "flare_frame", lara->flare.frame_num);
    JSONW_WRITE(io, "flare_control_left", lara->flare.control);
    JSONW_WRITE(io, "extra_anim", lara->extra_anim);
    // Introduced in TRX 1.2
    JSONW_WRITE(io, "vehicle_item_num", Lara_Vehicle_GetIndex());
    JSONW_WRITE(io, "vehicle_item_number", Lara_Vehicle_GetIndex());

    JSONW_WRITE(io, "mesh_effects", lara->mesh_effects);

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        JSONW_PUSH_VALUE(io, (int32_t)lara->wet[i]);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "wet");

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "skin_type", Lara_Skin_GetType());
    JSONW_WRITE(io, "skin_is_default", Lara_Skin_IsDefaultType());
    JSONW_WRITE(io, "holsters_visible", Lara_Skin_AreHolstersVisible());
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        const LARA_SKIN_EQUIPMENT *const equipment = Lara_Skin_GetEquipment(i);
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "type", equipment->type);
        JSONW_WRITE(io, "data", equipment->data);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "equipment");
    JSONW_POP_AND_SET(io, "skin");

    JSONW_WRITE(io, "target_angle1", lara->target_angles[0]);
    JSONW_WRITE(io, "target_angle2", lara->target_angles[1]);
    JSONW_WRITE(io, "turn_rate", lara->turn_rate);
    JSONW_WRITE(io, "move_angle", lara->move_angle);
    M_WriteXYZ16(io, "head_rot", lara->head_rot);
    M_WriteXYZ16(io, "torso_rot", lara->torso_rot);
    M_WriteXYZ32(io, "last_pos", lara->last_pos);
    M_WriteArm(io, "left_arm", &lara->left_arm);
    M_WriteArm(io, "right_arm", &lara->right_arm);
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        if (info->save_ammo_key != nullptr) {
            M_WriteAmmo(io, info->save_ammo_key, Inv_GetAmmo(info->gun_type));
        }
    }

    if (lara->gun_item_num != NO_ITEM) {
        JSONW_PUSH_OBJECT(io);
        const ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        SaveGame_WriteIdentity(
            io, "object_id", "object_key", CATALOG_OBJECTS,
            weapon_item->object_id);
        M_WriteAnimNum(io, weapon_item->anim_num);
        JSONW_WRITE(io, "frame_num", weapon_item->frame_num);
        JSONW_WRITE(io, "current_anim_state", weapon_item->current_anim_state);
        JSONW_WRITE(io, "goal_anim_state", weapon_item->goal_anim_state);
        JSONW_POP_AND_SET(io, "weapon");
    }

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "item_num", lara->interact_target.item_num);
    JSONW_WRITE(io, "move_count", lara->interact_target.move_count);
    JSONW_WRITE(io, "is_moving", lara->interact_target.is_moving);
    JSONW_POP_AND_SET(io, "interact_target");

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "index", lara->rope.index);
    JSONW_WRITE(io, "segment", lara->rope.segment);
    JSONW_WRITE(io, "direction", lara->rope.direction);
    JSONW_WRITE(io, "last_x_rot", lara->rope.last_x_rot);
    JSONW_WRITE(io, "arc_front", lara->rope.arc_front);
    JSONW_WRITE(io, "arc_back", lara->rope.arc_back);
    JSONW_WRITE(io, "max_x_forward", lara->rope.max_x_forward);
    JSONW_WRITE(io, "max_x_backward", lara->rope.max_x_backward);
    JSONW_WRITE(io, "d_frame", lara->rope.d_frame);
    JSONW_WRITE(io, "frame", lara->rope.frame);
    JSONW_WRITE(io, "frame_rate", lara->rope.frame_rate);
    JSONW_WRITE(io, "y_rot", lara->rope.y_rot);
    JSONW_WRITE(io, "offset", lara->rope.offset);
    JSONW_WRITE(io, "down_vel", lara->rope.down_vel);
    JSONW_WRITE(io, "flag", lara->rope.flag);
    JSONW_WRITE(io, "count", lara->rope.count);
    JSONW_POP_AND_SET(io, "rope");
    M_WriteRopeState(io);

    JSONW_POP_AND_SET(io, "lara");
}

void SG_File_DumpResumeInfoList(JSON_WRITE_IO *const io)
{
    const int32_t count = GF_GetLevelTable(GFLT_MAIN)->count;
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        const RESUME_INFO *const resume = SG_Resume_GetEntry(level);
        JSONW_PUSH_OBJECT(io);
        M_WriteResumeInfo(io, resume);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "resume_info");
}

void SG_File_DumpRules(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    JSON_OBJECT *const rules = JSON_WriteIO_GetCurrentObject(io);
    for (const RULE *rule = Rules_GetMap(); rule->name != nullptr; rule++) {
        if (Value_EqualPtr(rule->type, rule->target, rule->default_value)) {
            continue;
        }
        TRX_VALUE value = {};
        Value_ReadPtr(rule->type, rule->target, &value);
        JSONValue_Write(rules, rule->name, rule->type, nullptr, &value);
    }
    JSONW_POP_AND_SET_NZ(io, "rules");
}

void SG_File_DumpMisc(JSON_WRITE_IO *const io)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume = SG_Resume_GetEntry(level);

    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "game_version", g_TRXVersion);
    JSONW_WRITE(io, "bonus_flag", Game_GetBonusFlag());
    JSONW_WRITE(io, "death_count", resume->stats.death_count);
    JSONW_WRITE(io, "are_monks_angry", Creature_AreAlliesHostile());
    JSONW_WRITE(io, "sunset_timer", Output_GetTimeInGame());
    JSONW_WRITE(io, "rng_control_seed", Random_GetControlSeed());
    JSONW_WRITE(io, "rng_draw_seed", Random_GetDrawSeed());
    JSONW_WRITE(io, "weather_type", FX_Weather_GetWeather());
    const TRX_VALUE *const fog_color =
        Level_GetSettingOverride(LEVEL_SETTING_FOG_COLOR);
    if (fog_color != nullptr) {
        JSONW_WRITE(io, "fog_color", fog_color->as_rgb);
    }
    JSONW_WRITE(io, "cutscenes_played", CutSeq_GetPlayedMask());
    JSONW_WRITE(io, "waypoint", Waypoint_Get());
    JSONW_WRITE(io, "waypoint_highest", Waypoint_GetHighest());

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        if (Creature_IsAIObjectSpent(Item_Get(i))) {
            JSONW_PUSH_VALUE(io, i);
            JSONW_POP_AND_APPEND(io);
        }
    }
    JSONW_POP_AND_SET_NZ(io, "spent_ai_markers");
    JSONW_POP_AND_SET(io, "misc");

    JSONW_WRITE(io, "level_title", level->title != nullptr ? level->title : "");
    JSONW_WRITE(io, "save_counter", SG_Manager_GetCounter());
    JSONW_WRITE(io, "level_num", level->num);
}
