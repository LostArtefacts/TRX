#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/value.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/cutseq.h>
#include <trx/game/effects.h>
#include <trx/game/fx/common.h>
#include <trx/game/fx/weather.h>
#include <trx/game/game.h>
#include <trx/game/game_buf.h>
#include <trx/game/game_flow.h>
#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/flare_item.h>
#include <trx/game/objects/vars.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/rope.h>
#include <trx/game/rules.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#include <string.h>

#define M_SHOULD JSON_SHOULD
#define M_OPTIONAL JSON_OPTIONAL
#define M_MUST JSON_MUST
#define M_FAIL JSON_FAIL
#define M_FINISH JSON_FINISH

static bool M_ReadObjectID(
    JSON_READ_IO *const io, const char *const key, OBJECT_ID *const target)
{
    int32_t game_id = 0;
    M_MUST(JSON_READ(io, key, &game_id));
    *target = Object_FromGameID(game_id);
    if (*target == NO_OBJECT) {
        JSON_ReadIO_SetError(io, "unsupported object #%d", game_id);
        M_FAIL();
    }
    M_FINISH();
}

// Global anim indices shift as injections append anims, so prefer the
// object-relative reference where the save carries one.
static bool M_ReadAnimNum(JSON_READ_IO *const io, int16_t *const anim_num)
{
    const bool has_anim_num = JSON_READ(io, "anim_num", anim_num);

    int32_t game_id = 0;
    int32_t anim_rel = 0;
    if (M_OPTIONAL(JSON_READ(io, "anim_obj", &game_id))
        && M_OPTIONAL(JSON_READ(io, "anim_rel", &anim_rel))) {
        const OBJECT *const obj = Object_GetByGameID(game_id);
        if (obj != nullptr && obj->loaded && obj->anim_idx != NO_ANIM) {
            *anim_num = obj->anim_idx + anim_rel;
        }
    }

    return has_anim_num;
}

// Saves older than SG_VERSION_19 store a bare global anim index, which shifts
// when an object injected earlier gains anims.
// TODO: remove after 1.14
static void M_RepairItemAnim(ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    if (!obj->loaded || obj->anim_idx == NO_ANIM || obj->anim_count == 0) {
        return;
    }

    if (item->anim_num >= obj->anim_idx
        && item->anim_num < obj->anim_idx + obj->anim_count) {
        const ANIM *const anim = Item_GetAnim(item);
        if (item->frame_num >= anim->frame_base
            && item->frame_num <= anim->frame_end) {
            return;
        }
    }

    // The anim data itself did not move, and frame ranges are disjoint within
    // an object, so the saved frame identifies the anim and the offset into it.
    for (int16_t i = 0; i < obj->anim_count; i++) {
        const ANIM *const anim = Object_GetAnim(obj, i);
        if (item->frame_num < anim->frame_base
            || item->frame_num > anim->frame_end) {
            continue;
        }
        LOG_WARNING(
            "Item %d (object %d) has a stale anim %d; recovered anim %d from "
            "frame %d",
            Item_GetIndex(item), item->object_id, item->anim_num, i,
            item->frame_num);
        Item_SwitchToAnim(item, i, item->frame_num - anim->frame_base);
        item->current_anim_state = anim->current_anim_state;
        item->prev_frame_num = item->frame_num;
        return;
    }

    // The frame is unusable too, so fall back to the anim state.
    int16_t anim_idx = 0;
    for (int16_t i = 0; i < obj->anim_count; i++) {
        if (Object_GetAnim(obj, i)->current_anim_state
            == item->current_anim_state) {
            anim_idx = i;
            break;
        }
    }

    LOG_WARNING(
        "Item %d (object %d) has a stale anim %d and frame %d; resetting to "
        "anim %d",
        Item_GetIndex(item), item->object_id, item->anim_num, item->frame_num,
        anim_idx);
    Item_SwitchToAnim(item, anim_idx, 0);
    item->current_anim_state = Item_GetAnim(item)->current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->prev_frame_num = item->frame_num;
}

static bool M_ReadArm(
    JSON_READ_IO *const io, const char *const key, LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    M_MUST(JSON_PUSH(io, key));
    M_MUST(M_ReadAnimNum(io, &arm->anim_num));
    M_MUST(JSON_READ(io, "frame_num", &arm->frame_num));
    M_MUST(JSON_READ(io, "lock", &arm->lock));
    M_MUST(JSON_READ(io, "flash_gun", &arm->flash_gun));
    M_MUST(JSON_READ(io, "rot", &arm->rot));
    M_MUST(JSON_POP(io));
    M_FINISH();
}

static bool M_ReadAmmo(
    JSON_READ_IO *const io, const char *const key, const LARA_GUN_TYPE gun_type)
{
    int32_t ammo = 0;
    M_MUST(JSON_PUSH(io, key));
    M_MUST(JSON_READ(io, "ammo", &ammo));
    Inv_SetAmmo(gun_type, ammo);
    M_MUST(JSON_POP(io));
    M_FINISH();
}

static bool M_ReadXYZ32Array(
    JSON_READ_IO *const io, const char *const key, XYZ_32 *const target,
    const int32_t count)
{
    M_MUST(JSON_PUSH(io, key));
    if (JSON_ARRAY_LEN(io) != count) {
        JSON_ReadIO_SetError(
            io, "expected %d values in '%s', got %d", count, key,
            JSON_ARRAY_LEN(io));
        M_FAIL();
    }

    for (int32_t i = 0; i < count; i++) {
        M_MUST(JSON_READ_A(io, i, &target[i]));
    }
    M_MUST(JSON_POP(io));
    M_FINISH();
}

static bool M_ReadRopeState(JSON_READ_IO *const io)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    ROPE *const rope =
        lara->rope.index != NO_ROPE ? Rope_Get(lara->rope.index) : nullptr;
    if (rope == nullptr) {
        JSON_ReadIO_SetError(io, "invalid rope index %d", lara->rope.index);
        M_FAIL();
    }

    M_MUST(JSON_PUSH(io, "rope_state"));
    M_MUST(M_ReadXYZ32Array(io, "segments", rope->segments, ROPE_SEGMENTS));
    M_MUST(M_ReadXYZ32Array(io, "velocities", rope->velocities, ROPE_SEGMENTS));
    M_MUST(M_ReadXYZ32Array(
        io, "normalised_segments", rope->normalised_segments, ROPE_SEGMENTS));
    M_MUST(M_ReadXYZ32Array(
        io, "mesh_segments", rope->mesh_segments, ROPE_SEGMENTS));
    M_MUST(M_ReadXYZ32Array(
        io, "prev_mesh_segments", rope->prev_mesh_segments, ROPE_SEGMENTS));
    M_MUST(JSON_READ(io, "pos", &rope->pos));
    M_MUST(JSON_READ(io, "segment_length", &rope->segment_length));
    M_MUST(JSON_READ(io, "active", &rope->active));

    ROPE_PENDULUM *const pendulum = Rope_GetPendulum();
    M_MUST(JSON_PUSH(io, "pendulum"));
    M_MUST(JSON_READ(io, "pos", &pendulum->pos));
    M_MUST(JSON_READ(io, "vel", &pendulum->vel));
    M_MUST(JSON_READ(io, "node", &pendulum->node));
    M_MUST(JSON_POP(io));
    pendulum->rope = rope;

    M_MUST(JSON_POP(io));
    M_FINISH();
}

static bool M_ReadLara(JSON_READ_IO *const io)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    if (!M_OPTIONAL(JSON_READ(io, "item_number", &lara->item_num))) {
        // Introduced in TRX 1.2
        M_MUST(JSON_READ(io, "item_num", &lara->item_num));
    }
    M_MUST(JSON_READ(io, "gun_status", &lara->gun_status));
    M_MUST(JSON_READ(io, "gun_type", &lara->gun_type));
    M_MUST(JSON_READ(io, "request_gun_type", &lara->request_gun_type));

    // TRX <1.1
    if (g_TRVersion == 2 && JSON_ReadIO_GetVersion(io) < SG_VERSION_14) {
        if (lara->gun_type == LGT_MAGNUMS) {
            lara->gun_type = LGT_AUTOS;
        }
        if (lara->request_gun_type == LGT_MAGNUMS) {
            lara->request_gun_type = LGT_AUTOS;
        }
    }

    M_MUST(JSON_READ(io, "last_gun_type", &lara->last_gun_type));
    M_MUST(JSON_READ(io, "calc_fall_speed", &lara->calc_fall_speed));
    M_MUST(JSON_READ(io, "water_status", &lara->water_status));
    M_MUST(JSON_READ(io, "climb_status", &lara->climb_status));
    M_SHOULD(JSON_READ(io, "corner_pos_x", &lara->corner_pos.x));
    M_SHOULD(JSON_READ(io, "corner_pos_z", &lara->corner_pos.z));
    M_SHOULD(JSON_READ(io, "is_crouched", &lara->is_crouched));
    M_SHOULD(JSON_READ(io, "keep_crouched", &lara->keep_crouched));
    M_SHOULD(JSON_READ(io, "sprinting", &lara->sprinting));
    M_MUST(JSON_READ(io, "pose_count", &lara->pose_count));
    M_MUST(JSON_READ(io, "hit_frame", &lara->hit_frame));
    M_MUST(JSON_READ(io, "hit_direction", &lara->hit_direction));
    M_MUST(JSON_READ(io, "air", &lara->air));
    M_MUST(JSON_READ(io, "sprint_timer", &lara->sprint_timer));
    M_MUST(JSON_READ(io, "exposure_timer", &lara->exposure_timer));
    M_SHOULD(JSON_READ(io, "poison_timer", &lara->poison.value));
    M_SHOULD(JSON_READ(io, "poison_target", &lara->poison.target));
    M_MUST(JSON_READ(io, "dive_count", &lara->dive_timer));
    M_MUST(JSON_READ(io, "death_count", &lara->death_timer));
    M_MUST(JSON_READ(io, "current_active", &lara->current.active));
    M_SHOULD(JSON_READ(io, "current_vel_x", &lara->current.vel.x));
    M_SHOULD(JSON_READ(io, "current_vel_z", &lara->current.vel.z));
    M_MUST(JSON_READ(io, "burn", &lara->burn));
    // Introduced in TRX 1.2
    M_SHOULD(JSON_READ(io, "electric", &lara->electric));

    M_MUST(JSON_READ(io, "mesh_effects", &lara->mesh_effects));

    // Introduced in TRX 1.10
    memset(lara->wet, 0, sizeof(lara->wet));
    if (M_OPTIONAL(JSON_PUSH(io, "wet"))) {
        const int32_t count = MIN(JSON_ARRAY_LEN(io), LM_NUMBER_OF);
        for (int32_t i = 0; i < count; i++) {
            int32_t value = 0;
            M_MUST(JSON_READ_A(io, i, &value));
            lara->wet[i] = value;
        }
        M_MUST(JSON_POP(io));
    }
    M_MUST(JSON_READ(io, "extra_anim", &lara->extra_anim));
    M_MUST(JSON_READ(io, "water_surface_dist", &lara->water_surface_dist));

    M_MUST(JSON_READ(io, "hit_effect_count", &lara->hit_effect_count));
    int16_t hit_effect = NO_EFFECT;
    M_MUST(JSON_READ(io, "hit_effect", &hit_effect));
    lara->hit_effect =
        hit_effect != NO_EFFECT && g_Config.gameplay.enable_enhanced_saves
        ? Effect_Get(hit_effect)
        : nullptr;

    int16_t vehicle_idx = Lara_Vehicle_GetIndex();
    if (!M_OPTIONAL(JSON_READ(io, "vehicle_item_number", &vehicle_idx))) {
        // Introduced in TRX 1.2
        M_MUST(JSON_READ(io, "vehicle_item_num", &vehicle_idx));
    }
    Lara_Vehicle_SetIndex(vehicle_idx);

    M_MUST(JSON_READ(io, "flare_age", &lara->flare.age));
    M_MUST(JSON_READ(io, "flare_frame", &lara->flare.frame_num));
    M_MUST(JSON_READ(io, "flare_control_left", &lara->flare.control));

    M_MUST(JSON_PUSH(io, "skin"));
    LARA_SKIN_TYPE skin_type = LARA_SKIN_TYPE_DEFAULT;
    bool skin_is_default = false;
    M_MUST(JSON_READ(io, "skin_type", &skin_type));
    M_MUST(JSON_READ(io, "skin_is_default", &skin_is_default));
    if (!skin_is_default) {
        Lara_Skin_SetType(skin_type);
    }

    bool holsters_visible = true;
    M_MUST(JSON_READ(io, "holsters_visible", &holsters_visible));
    Lara_Skin_SetHolstersVisible(holsters_visible);

    M_MUST(JSON_PUSH(io, "equipment"));
    const int32_t mesh_count = JSON_ARRAY_LEN(io);
    if (mesh_count != LM_NUMBER_OF) {
        JSON_ReadIO_SetError(
            io, "expected %d equipment meshes, got %d", LM_NUMBER_OF,
            mesh_count);
        M_FAIL();
    }
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        LARA_SKIN_EQUIPMENT_TYPE type = EQUIPMENT_TYPE_NONE;
        int32_t data = -1;
        M_MUST(JSON_PUSH_INDEX(io, i));
        M_MUST(JSON_READ(io, "type", &type));
        M_MUST(JSON_READ(io, "data", &data));
        M_MUST(JSON_POP(io));

        if (type == EQUIPMENT_TYPE_WEAPON) {
            Lara_Skin_SetGunEquipment(i, data);
        } else if (type == EQUIPMENT_TYPE_EXTRA) {
            Lara_Skin_SetExtraEquipment(i, data);
        } else {
            Lara_Skin_ClearEquipment(i);
        }
    }
    M_MUST(JSON_POP(io));
    M_MUST(JSON_POP(io));
    Lara_Skin_ApplyOutfit();

    lara->target = nullptr;
    M_MUST(JSON_READ(io, "target_angle1", &lara->target_angles[0]));
    M_MUST(JSON_READ(io, "target_angle2", &lara->target_angles[1]));
    M_MUST(JSON_READ(io, "turn_rate", &lara->turn_rate));
    M_MUST(JSON_READ(io, "move_angle", &lara->move_angle));
    M_MUST(JSON_READ(io, "head_rot", &lara->head_rot));
    M_MUST(JSON_READ(io, "torso_rot", &lara->torso_rot));
    M_MUST(JSON_READ(io, "last_pos", &lara->last_pos));

    // Arms need no repair; the gun control recomputes them every frame.
    M_MUST(M_ReadArm(io, "left_arm", &lara->left_arm));
    M_MUST(M_ReadArm(io, "right_arm", &lara->right_arm));
    for (const SAVEGAME_AMMO_ENTRY *entry = g_Savegame_WeaponAmmo;
         entry->key != nullptr; entry++) {
        if (entry->required) {
            M_MUST(M_ReadAmmo(io, entry->key, entry->gun_type));
        } else {
            M_SHOULD(M_ReadAmmo(io, entry->key, entry->gun_type));
        }
    }

    if (M_OPTIONAL(JSON_PUSH(io, "weapon"))) {
        lara->gun_item_num = Item_Create();
        ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        weapon_item->is_visible = true;
        weapon_item->room_num = NO_ROOM;
        // Introduced in TRX 1.2
        if (!M_SHOULD(
                M_ReadObjectID(io, "object_id", &weapon_item->object_id))) {
            M_MUST(M_ReadObjectID(io, "obj_id", &weapon_item->object_id));
        }
        M_MUST(M_ReadAnimNum(io, &weapon_item->anim_num));
        M_MUST(JSON_READ(io, "frame_num", &weapon_item->frame_num));
        M_MUST(JSON_READ(
            io, "current_anim_state", &weapon_item->current_anim_state));
        M_MUST(JSON_READ(io, "goal_anim_state", &weapon_item->goal_anim_state));
        M_MUST(JSON_POP(io));
        if (JSON_ReadIO_GetVersion(io) < SG_VERSION_19) {
            M_RepairItemAnim(weapon_item);
        }
    }

    M_MUST(JSON_PUSH(io, "interact_target"));
    M_MUST(JSON_READ(io, "item_num", &lara->interact_target.item_num));
    M_MUST(JSON_READ(io, "move_count", &lara->interact_target.move_count));
    M_MUST(JSON_READ(io, "is_moving", &lara->interact_target.is_moving));
    M_MUST(JSON_POP(io));

    // Introduced with the TR4 rope; missing in older saves.
    lara->rope.index = NO_ROPE;
    if (M_OPTIONAL(JSON_PUSH(io, "rope"))) {
        M_MUST(JSON_READ(io, "index", &lara->rope.index));
        M_MUST(JSON_READ(io, "segment", &lara->rope.segment));
        M_MUST(JSON_READ(io, "direction", &lara->rope.direction));
        M_MUST(JSON_READ(io, "last_x_rot", &lara->rope.last_x_rot));
        M_MUST(JSON_READ(io, "arc_front", &lara->rope.arc_front));
        M_MUST(JSON_READ(io, "arc_back", &lara->rope.arc_back));
        M_MUST(JSON_READ(io, "max_x_forward", &lara->rope.max_x_forward));
        M_MUST(JSON_READ(io, "max_x_backward", &lara->rope.max_x_backward));
        M_MUST(JSON_READ(io, "d_frame", &lara->rope.d_frame));
        M_MUST(JSON_READ(io, "frame", &lara->rope.frame));
        M_MUST(JSON_READ(io, "frame_rate", &lara->rope.frame_rate));
        M_MUST(JSON_READ(io, "y_rot", &lara->rope.y_rot));
        M_MUST(JSON_READ(io, "offset", &lara->rope.offset));
        M_MUST(JSON_READ(io, "down_vel", &lara->rope.down_vel));
        M_MUST(JSON_READ(io, "flag", &lara->rope.flag));
        M_MUST(JSON_READ(io, "count", &lara->rope.count));
        M_MUST(JSON_POP(io));
    }
    if (JSON_ReadIO_HasKey(io, "rope_state")) {
        M_MUST(M_ReadRopeState(io));
    }

    M_FINISH();
}

static bool M_IsValidItemObject(
    const OBJECT_ID saved_obj_id, const OBJECT_ID initial_obj_id)
{
    if (saved_obj_id == initial_obj_id) {
        return true;
    }
    if (Object_IsType(initial_obj_id, g_GunObjects)
        && Object_IsType(saved_obj_id, g_GunObjects)) {
        return true;
    }

    // used receptacles
    if (Object_GetCognate(initial_obj_id, g_ReceptacleToReceptacleDoneMap)
        == saved_obj_id) {
        return true;
    }
    // ammo left behind by an already collected gun
    if (Object_GetCognateInverse(saved_obj_id, g_GunAmmoObjectMap)
        == initial_obj_id) {
        return true;
    }

    // clang-format off
    switch (saved_obj_id) {
        // dual-state animals
        case O_ALLIGATOR: return initial_obj_id == O_CROCODILE;
        case O_CROCODILE: return initial_obj_id == O_ALLIGATOR;
        case O_RAT: return initial_obj_id == O_VOLE;
        case O_VOLE: return initial_obj_id == O_RAT;
        // skidoo swaps
        case O_SKIDOO_FAST: return initial_obj_id == O_SKIDOO_ARMED;
        // default
        default: return false;
    }
    // clang-format on
}

static int16_t M_ResolveItem(JSON_READ_IO *const io, const int16_t read_index)
{
    const char *item_name = nullptr;
    if (M_OPTIONAL(JSON_READ(io, "name", &item_name))) {
        const ITEM *const item = Item_GetByName(item_name);
        if (item == nullptr) {
            LOG_WARNING(
                "invalid item name '%s' (read index %d)", item_name,
                read_index);
            return NO_ITEM;
        }

        return Item_GetIndex(item);
    }

    int16_t item_num;
    if (!M_SHOULD(JSON_READ(io, "index", &item_num))) {
        item_num = read_index; // TODO: remove after TRX 2.0
    }

    if (item_num < 0 || item_num >= Item_GetLevelCount()) {
        LOG_WARNING(
            "invalid item index %d (read index %d)", item_num, read_index);
        return NO_ITEM;
    }

    return item_num;
}

static bool M_ShouldReadAnim(const OBJECT *const obj, const OBJECT_ID obj_id)
{
    return obj->save_anim
        && (obj_id != O_SPIKES || g_Config.gameplay.fix_animated_spikes);
}

static bool M_ShouldReadFlags(const OBJECT *const obj, const OBJECT_ID obj_id)
{
    return obj->save_flags
        && (obj_id != O_SPIKES || g_Config.gameplay.fix_animated_spikes);
}

static bool M_ReadItem(JSON_READ_IO *const io, const int16_t read_index)
{
    const int16_t item_num = M_ResolveItem(io, read_index);
    if (item_num == NO_ITEM) {
        // soft exit for unresolvable items
        return true;
    }

    ITEM *const item = Item_Get(item_num);

    OBJECT_ID object_id = NO_OBJECT;
    M_MUST(M_ReadObjectID(io, "object_id", &object_id));

    const OBJECT *const obj = Object_Get(object_id);
    item->object_id = object_id;
    if (!M_IsValidItemObject(object_id, item->object_id)) {
        JSON_ReadIO_SetError(
            io, "level has %d (%s), save has %d (%s)", item->object_id,
            Object_GetName(item->object_id), object_id,
            Object_GetName(object_id));
        M_FAIL();
    }

    {
        // Introduced in TRX 1.9
        int32_t mesh_bits;
        if (M_SHOULD(JSON_READ(io, "mesh_bits", &mesh_bits))) {
            item->mesh_bits = (uint32_t)mesh_bits;
        }
    }

    // Not sure why some items do not have their their position saved,
    // despite OBJECT telling them to.
    if (obj->save_position && JSON_ReadIO_HasKey(io, "room_num")) {
        M_MUST(JSON_READ(io, "pos", &item->pos));
        M_MUST(JSON_READ(io, "rot", &item->rot));
        M_MUST(JSON_READ(io, "speed", &item->speed));
        M_MUST(JSON_READ(io, "fall_speed", &item->fall_speed));
        int16_t room_num = NO_ROOM;
        M_MUST(JSON_READ(io, "room_num", &room_num));
        if (room_num != NO_ROOM) {
            Item_UpdateRoom(item_num, room_num);
        }
    }

    if (M_ShouldReadAnim(obj, object_id)) {
        // TRX >= 1.1 animated puzzle holes became animated
        M_SHOULD(JSON_READ(io, "current_anim", &item->current_anim_state));
        M_SHOULD(JSON_READ(io, "goal_anim", &item->goal_anim_state));
        M_SHOULD(JSON_READ(io, "required_anim", &item->required_anim_state));
        M_SHOULD(M_ReadAnimNum(io, &item->anim_num));
        M_SHOULD(JSON_READ(io, "frame_num", &item->frame_num));
        M_SHOULD(JSON_READ(io, "prev_frame_num", &item->prev_frame_num));

        // Prevent issues with pre-injection saves and Lara's enhanced
        // animation set.
        if (item->object_id == O_LARA
            && item->anim_num < LARA_ORIGINAL_ANIM_COUNT) {
            item->anim_num += obj->anim_idx;
        }

        // Lara's anim may belong to a vehicle rather than to herself.
        if (JSON_ReadIO_GetVersion(io) < SG_VERSION_19
            && item->object_id != O_LARA) {
            M_RepairItemAnim(item);
        }
    }

    if (obj->save_hitpoints) {
        int16_t hit_points = 0;
        int16_t max_hit_points = 0;
        M_MUST(JSON_READ(io, "hitpoints", &hit_points));
        M_MUST(JSON_READ(io, "max_hitpoints", &max_hit_points));
        ObjectProperty_SetItemValueRaw(
            item, "max_hit_points",
            (TRX_VALUE) {
                .type = TVT_S32,
                .as_int = max_hit_points,
            });
        item->hit_points = hit_points;
    }
    M_MUST(ObjectProperty_ReadItemOverrides(io, item));

    if (M_ShouldReadFlags(obj, object_id)) {
        if (!JSON_ReadIO_HasKey(io, "flags")) {
            // TRX 1.1 save-crystal entries were serialized as bare items
            // without save-state fields. Treat them as default-state crystals
            // so those legacy saves remain loadable.
            if (object_id == O_SAVE_CRYSTAL_ITEM) {
                goto skip_flags;
            }
        }
        // TRX 1.8 introduced fixing animated spikes on load
        uint16_t flags = 0;
        M_SHOULD(JSON_READ(io, "flags", &flags));
        item->trigger = (ITEM_TRIGGER_STATE) {
            .mask = (flags & IF_CODE_BITS) >> TRIGGER_MASK_SHIFT,
            .reversed = (flags & IF_REVERSE) != 0,
            .switch_spent = (flags & IF_ONE_SHOT_SWITCH) != 0,
            .anti_spent = (flags & IF_ONE_SHOT_ANTITRIGGER) != 0,
        };
        item->is_destroyed = (flags & IF_DESTROYED) != 0;
        item->trigger.spent = (flags & IF_ONE_SHOT) != 0;
        M_SHOULD(JSON_READ(io, "timer", &item->timer));
        // Unpack the released save format's status into the visibility and
        // finished axes; is_simulated comes from the separate "active" field
        // below. A missing key leaves the level-load axes untouched.
        int32_t saved_status = -1;
        M_SHOULD(JSON_READ(io, "status", &saved_status));
        if (saved_status >= 0) {
            item->is_visible = saved_status != IS_INVISIBLE;
            item->is_finished = saved_status == IS_DEACTIVATED;
        }
        // Written since the axes split; a hidden finished item packs to
        // IS_INVISIBLE, so the status alone would drop the marker. Released
        // saves lack the key and keep the value derived above.
        M_OPTIONAL(JSON_READ(io, "finished", &item->is_finished));

        if (item->is_destroyed) {
            Item_Destroy(item_num);
        } else {
            bool is_active = false;
            M_SHOULD(JSON_READ(io, "active", &is_active));
            if (is_active && !item->is_simulated) {
                Item_AddSimulated(item_num);
                // Item_AddSimulated skips control-less items, which cannot
                // join the simulation list; they still carry the axis, set
                // where something else simulates them (the skidoo the driver
                // puppets, in skidoo_driver.c).
                item->is_simulated = true;
            } else if (
                !is_active && saved_status == IS_ACTIVE
                && Object_IsType(item->object_id, g_ReceptacleObjects)) {
                // A released save recorded a receptacle's armed "key inserted"
                // state as IS_ACTIVE without the active bit - the keyhole was
                // control-less and never joined the active list. New saves
                // carry active=true and take the branch above. Re-arm it so
                // the pending key trigger still fires (Keyhole_Trigger reads
                // Item_IsInPlay, which needs is_simulated).
                Item_AddSimulated(item_num);
            }
            M_SHOULD(JSON_READ(io, "gravity", &item->gravity));
            // Introduced in TRX 1.2
            M_OPTIONAL(JSON_READ(io, "collidable", &item->is_collidable));
        }
        // Introduced in TRX 1.2, not written if zero
        M_OPTIONAL(JSON_READ(io, "ai_bits", &item->ai_bits));
        M_OPTIONAL(JSON_READ(io, "ai_tag", &item->ai_tag));
        // Introduced in TRX 1.10, not written if zero
        M_OPTIONAL(JSON_READ(io, "fade", &item->fade));

        bool intelligent = obj->intelligent;
        // Introduced in TRX 1.2
        M_SHOULD(JSON_READ(io, "intelligent", &intelligent));
        if (intelligent) {
            LOT_EnableBaddieAI(item_num, true);
            CREATURE *const creature = item->creature_data;
            if (creature != nullptr) {
                M_MUST(JSON_READ(io, "head_rot", &creature->head_rotation));
                M_MUST(JSON_READ(io, "neck_rot", &creature->neck_rotation));
                M_MUST(JSON_READ(io, "max_turn", &creature->maximum_turn));
                M_MUST(JSON_READ(io, "creature_flags", &creature->flags));
                M_MUST(JSON_READ(io, "creature_mood", &creature->mood));
                if (M_SHOULD(JSON_PUSH(io, "creature"))) {
                    // Introduced in TRX 1.2
                    M_MUST(JSON_READ(io, "alerted", &creature->alerted));
                    M_MUST(JSON_READ(io, "head_left", &creature->head_left));
                    M_MUST(JSON_READ(io, "head_right", &creature->head_right));
                    M_MUST(
                        JSON_READ(io, "reached_goal", &creature->reached_goal));
                    M_MUST(JSON_READ(io, "patrol_2", &creature->patrol_2));
                    M_MUST(
                        JSON_READ(io, "hurt_by_lara", &creature->hurt_by_lara));
                    M_MUST(JSON_READ(
                        io, "damage_from_lara", &creature->damage_from_lara));
                    M_MUST(JSON_PUSH(io, "joint_rotations"));
                    for (int32_t i = 0; i < 4; i++) {
                        // Introduced in TRX 1.2
                        M_SHOULD(
                            JSON_READ_A(io, i, &creature->joint_rotation[i]));
                    }
                    M_MUST(JSON_POP(io));
                    M_MUST(JSON_POP(io));
                }
            }
        } else if (obj->intelligent) {
            item->creature_data = nullptr;
            item->extra_rotations = nullptr;
        }
    }
skip_flags:

    if (M_SHOULD(JSON_PUSH(io, "carried_items"))) {
        CARRIED_ITEM *carried_item = item->carried_item;
        CARRIED_ITEM *prev_item = nullptr;
        for (int32_t j = 0;; j++) {
            if (!JSON_PUSH_INDEX(io, j)) {
                break;
            }
            if (carried_item == nullptr) {
                carried_item = GameBuf_Alloc(sizeof(CARRIED_ITEM), GBUF_ITEMS);
                carried_item->next_item = nullptr;
                carried_item->spawn_num = NO_ITEM;
                if (prev_item != nullptr) {
                    prev_item->next_item = carried_item;
                } else {
                    item->carried_item = carried_item;
                }
            }
            // Introduced in TRX 1.2. Must be read for both newly allocated and
            // pre-existing carried entries (e.g. gameflow-defined drops).
            M_SHOULD(JSON_READ(io, "spawn_num", &carried_item->spawn_num));

            M_MUST(M_ReadObjectID(io, "object_id", &carried_item->object_id));
            M_MUST(JSON_READ(io, "pos", &carried_item->pos));
            M_MUST(JSON_READ(io, "y_rot", &carried_item->rot.y));
            M_MUST(JSON_READ(io, "room_num", &carried_item->room_num));
            M_MUST(JSON_READ(io, "fall_speed", &carried_item->fall_speed));
            M_MUST(JSON_READ(io, "status", &carried_item->status));

            Carrier_SyncItem(item_num, carried_item);

            prev_item = carried_item;
            carried_item = carried_item->next_item;
            M_MUST(JSON_POP(io));
        }
        M_MUST(JSON_POP(io));
    }

    if (obj->priv_size > 0 && obj->priv_load_func != nullptr) {
        // "priv" introduced in TRX 1.2
        if (M_SHOULD(JSON_PUSH(io, "priv"))
            || M_SHOULD(JSON_PUSH(io, "data"))) {
            obj->priv_load_func(item, io);
            M_MUST(JSON_POP(io));
        }
    }

    if (g_TRVersion >= 2) {
        // TODO: make this call in both engines consistently
        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
        }
    }

    M_FINISH();
}

static bool M_ReadEffect(JSON_READ_IO *const io)
{
    int32_t room_num = NO_ROOM;
    if (!M_OPTIONAL(JSON_READ(io, "room_number", &room_num))) {
        // Introduced in TRX 1.2
        M_MUST(JSON_READ(io, "room_num", &room_num));
    }

    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_EFFECT) {
        return true;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    M_MUST(JSON_READ(io, "pos", &effect->pos));
    M_MUST(JSON_READ(io, "rot", &effect->rot));
    if (!M_OPTIONAL(M_ReadObjectID(io, "object_number", &effect->object_id))) {
        // Introduced in TRX 1.2
        M_MUST(M_ReadObjectID(io, "object_id", &effect->object_id));
    }
    M_MUST(JSON_READ(io, "speed", &effect->speed));
    M_MUST(JSON_READ(io, "fall_speed", &effect->fall_speed));
    if (!M_OPTIONAL(JSON_READ(io, "frame_number", &effect->frame_num))) {
        // Introduced in TRX 1.2
        M_MUST(JSON_READ(io, "frame_num", &effect->frame_num));
    }
    M_MUST(JSON_READ(io, "counter", &effect->counter));
    M_MUST(JSON_READ(io, "shade", &effect->shade));
    JSON_SHOULD(JSON_READ(io, "flag1", &effect->flag1));
    JSON_SHOULD(JSON_READ(io, "flag2", &effect->flag2));
    M_FINISH();
}

static bool M_ReadFlare(JSON_READ_IO *const io)
{
    const int16_t item_num = Item_Create();
    ITEM *const item = Item_Get(item_num);
    item->object_id = O_FLARE_ITEM;
    M_MUST(JSON_READ(io, "pos", &item->pos));
    M_MUST(JSON_READ(io, "rot", &item->rot));
    M_MUST(JSON_READ(io, "room_num", &item->room_num));
    Item_Initialise(item_num);
    M_MUST(JSON_READ(io, "speed", &item->speed));
    M_MUST(JSON_READ(io, "fall_speed", &item->fall_speed));
    int32_t flare_age;
    M_MUST(JSON_READ(io, "age", &flare_age));
    FlareItem_SetAge(item, flare_age & 0x7FFF, (flare_age & 0x8000) != 0);
    Item_AddSimulated(item_num);
    M_FINISH();
}

// Returns the timestamp to resume the track at, or -1.0 to play it from the
// start.
static double M_GetMusicSeekTimestamp(
    const MUSIC_ID track_id, const MUSIC_PLAY_MODE mode,
    const MUSIC_ID ambient_track, const double timestamp)
{
    const bool is_ambient = mode == MPM_LOOP && track_id == ambient_track;
    if (is_ambient
        && g_Config.audio.music_load_condition != MUSIC_LOAD_CONDITION_ALWAYS) {
        return -1.0;
    }
    return timestamp;
}

static bool M_ReadMusicTracks(JSON_READ_IO *const io)
{
    MUSIC_ID ambient_track = MX_INACTIVE;
    M_MUST(JSON_READ(io, "current_ambient", &ambient_track));

    Music_Stop();
    if (ambient_track != MX_INACTIVE) {
        // Always restart the ambient as it may have changed based on the
        // current position in the level.
        Music_Play_Direct(ambient_track, MPM_LOOP);
    }

    if (g_Config.audio.music_load_condition == MUSIC_LOAD_CONDITION_NEVER) {
        return true;
    }

    if (M_SHOULD(JSON_PUSH(io, "streams"))) {
        // TRX 1.2
        const int32_t stream_count = JSON_ARRAY_LEN(io);
        for (int32_t i = 0; i < stream_count; i++) {
            MUSIC_ID track_id = MX_INACTIVE;
            MUSIC_PLAY_MODE mode = MPM_ONCE;
            double timestamp = -1.0;
            M_MUST(JSON_PUSH_INDEX(io, i));
            M_MUST(JSON_READ(io, "track", &track_id));
            M_MUST(JSON_READ(io, "mode", &mode));
            M_MUST(JSON_READ(io, "timestamp", &timestamp));
            M_MUST(JSON_POP(io));

            if (track_id == MX_INACTIVE) {
                continue;
            }
            const double seek_to = M_GetMusicSeekTimestamp(
                track_id, mode, ambient_track, timestamp);
            if (Music_Play_DirectAt(track_id, mode, seek_to) < 0) {
                LOG_WARNING(
                    "Could not load stream track %d at timestamp %lf.",
                    track_id, timestamp);
            }
        }
        M_MUST(JSON_POP(io));
    } else {
        MUSIC_ID current_track = MX_INACTIVE;
        double timestamp = -1.0;
        M_MUST(JSON_READ(io, "current_track", &current_track));
        M_MUST(JSON_READ(io, "timestamp", &timestamp));

        const bool is_ambient = current_track == ambient_track;
        const MUSIC_PLAY_MODE mode = is_ambient ? MPM_LOOP : MPM_ONCE;
        const double seek_to = M_GetMusicSeekTimestamp(
            current_track, mode, ambient_track, timestamp);
        if (current_track != MX_INACTIVE
            && Music_Play_DirectAt(current_track, mode, seek_to) < 0) {
            LOG_WARNING(
                "Could not load current track %d at timestamp %lf.",
                current_track, timestamp);
        }
    }

    M_FINISH();
}

static bool M_ReadMusicTrackFlags(JSON_READ_IO *const io)
{
    if (!g_Config.audio.load_music_triggers) {
        return true;
    }

    const int32_t count = JSON_ARRAY_LEN(io);
    if (count > MAX_MUSIC_TRACKS) {
        JSON_ReadIO_SetError(
            io, "expected at most %d music track flags, got %d",
            MAX_MUSIC_TRACKS, count);
        M_FAIL();
    }

    for (int32_t i = 0; i < count; i++) {
        uint32_t flags;
        M_MUST(JSON_READ_A(io, i, &flags));
        MUSIC_TRACK_STATE *const track = Music_GetTrackState(i);
        track->mask = (flags & MTF_CODE_BITS) >> TRIGGER_MASK_SHIFT;
        track->is_one_shot = (flags & MTF_ONE_SHOT) != 0;
        track->delay = flags & 0xFF;
    }

    M_FINISH();
}

static bool M_ReadResumeInfo(JSON_READ_IO *const io, RESUME_INFO *const resume)
{
    resume->lara_hitpoints = g_Config.gameplay.start_lara_hitpoints;
    M_MUST(JSON_READ(io, "lara_hitpoints", &resume->lara_hitpoints));
    M_MUST(JSON_READ(io, "gun_status", &resume->gun_status)); // LGS_ARMLESS
    M_MUST(
        JSON_READ(io, "gun_type", &resume->equipped_gun_type)); // LGT_UNARMED
    M_MUST(JSON_READ(
        io, "holsters_gun_type",
        &resume->holsters_gun_type)); // LGT_UNKNOWN

    // TRX <1.1
    if (g_TRVersion == 2 && JSON_ReadIO_GetVersion(io) < SG_VERSION_14) {
        if (resume->equipped_gun_type == LGT_MAGNUMS) {
            resume->equipped_gun_type = LGT_AUTOS;
        }
        if (resume->holsters_gun_type == LGT_MAGNUMS) {
            resume->holsters_gun_type = LGT_AUTOS;
        }
    }

    M_MUST(
        JSON_READ(io, "back_gun_type", &resume->back_gun_type)); // LGT_UNKNOWN
    M_MUST(JSON_READ(io, "costume", &resume->flags.costume));

    for (const SAVEGAME_RESUME_WEAPON *entry = g_Savegame_ResumeWeapons;
         entry->has_key != nullptr; entry++) {
        int32_t ammo = 0;
        bool has_weapon = false;
        if (entry->required) {
            M_MUST(JSON_READ(io, entry->ammo_key, &ammo));
            M_MUST(JSON_READ(io, entry->has_key, &has_weapon));
        } else {
            M_SHOULD(JSON_READ(io, entry->ammo_key, &ammo));
            M_SHOULD(JSON_READ(io, entry->has_key, &has_weapon));
        }
        resume->inv.ammo[entry->gun_type] = ammo;
        Inv_State_SetCount(
            &resume->inv, Gun_GetGunObject(entry->gun_type),
            has_weapon ? 1 : 0);
    }

    // Introduced in TRX 1.9
    bool has_binoculars = false;
    M_SHOULD(JSON_READ(io, "has_binoculars", &has_binoculars));
    Inv_State_SetCount(&resume->inv, O_BINOCULARS_ITEM, has_binoculars ? 1 : 0);

    for (const SAVEGAME_RESUME_ITEM *entry = g_Savegame_ResumeItems;
         entry->key != nullptr; entry++) {
        int32_t qty = 0;
        if (entry->required) {
            M_MUST(JSON_READ(io, entry->key, &qty));
        } else {
            M_SHOULD(JSON_READ(io, entry->key, &qty));
        }
        Inv_State_SetCount(&resume->inv, entry->object_id, qty);
    }

    M_MUST(JSON_READ(io, "available", &resume->flags.available));

    // Introduced in TRX 1.2
    resume->level_completed = false;
    resume->prev_level = -1;
    resume->hurt_allies = false;
    M_SHOULD(JSON_READ(io, "level_completed", &resume->level_completed));
    M_SHOULD(JSON_READ(io, "prev_level", &resume->prev_level));
    M_SHOULD(JSON_READ(io, "hurt_allies", &resume->hurt_allies));

    // Introduced in TRX 1.10
    resume->burning = false;
    M_SHOULD(JSON_READ(io, "burning", &resume->burning));

    M_MUST(JSON_READ(io, "timer", &resume->stats.timer));
    M_MUST(JSON_READ(io, "ammo_hits", &resume->stats.ammo_hits));
    M_MUST(JSON_READ(io, "ammo_used", &resume->stats.ammo_used));
    M_MUST(JSON_READ(io, "medipacks_used", &resume->stats.medipacks_used));
    M_MUST(
        JSON_READ(io, "distance_travelled", &resume->stats.distance_travelled));
    M_MUST(JSON_READ(io, "kills", &resume->stats.counts[STATS_CAT_KILLS]));
    M_SHOULD(
        JSON_READ(io, "crystals", &resume->stats.counts[STATS_CAT_CRYSTALS]));
    M_MUST(JSON_READ(io, "pickups", &resume->stats.counts[STATS_CAT_PICKUPS]));
    M_MUST(JSON_READ(io, "secrets", &resume->stats.secret_flags));
    M_SHOULD(JSON_READ(io, "death_count", &resume->stats.death_count));
    Stats_UpdateSecrets(&resume->stats);
    M_FINISH();
}

bool SG_File_LoadInventory(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "inventory"));
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();

    Lara_InitialiseInventory(current_level);
    for (int32_t i = 0; g_Savegame_InventoryItems[i].key != nullptr; i++) {
        int16_t qty;
        if (JSON_READ(io, g_Savegame_InventoryItems[i].key, &qty)) {
            while (Inv_GetItemCount(g_Savegame_InventoryItems[i].object_id)
                   != 0) {
                Inv_RemoveItem(g_Savegame_InventoryItems[i].object_id);
            }
            Inv_AddItemNTimes(g_Savegame_InventoryItems[i].object_id, qty);
        }
    }

    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadFlipmaps(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "flipmap"));

    bool status;
    M_MUST(JSON_READ(io, "status", &status));
    if (status) {
        Room_FlipMap();
    }

    int32_t flip_effect;
    int32_t flip_timer;
    M_MUST(JSON_READ(io, "effect", &flip_effect));
    M_MUST(JSON_READ(io, "timer", &flip_timer));
    Room_SetFlipEffect(flip_effect);
    Room_SetFlipTimer(flip_timer);

    M_MUST(JSON_PUSH(io, "table"));
    const size_t count = JSON_ARRAY_LEN(io);
    if (count != MAX_FLIP_MAPS) {
        JSON_ReadIO_SetError(
            io, "expected %d flipmap elements, got %d", MAX_FLIP_MAPS, count);
        M_FAIL();
    }
    for (size_t i = 0; i < count; i++) {
        uint32_t flags;
        M_MUST(JSON_READ_A(io, i, &flags));
        FLIP_SLOT *const slot = Room_GetFlipSlot(i);
        slot->mask = ((flags << 8) & FSF_CODE_BITS) >> TRIGGER_MASK_SHIFT;
        slot->is_one_shot = ((flags << 8) & FSF_ONE_SHOT) != 0;
    }
    M_MUST(JSON_POP(io));

    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadCameras(JSON_READ_IO *const io)
{
    {
        M_MUST(JSON_PUSH(io, "cameras"));
        const size_t count = JSON_ARRAY_LEN(io);
        if (count != (size_t)Camera_GetFixedObjectCount()) {
            JSON_ReadIO_SetError(
                io, "expected %d cameras, got %d", Camera_GetFixedObjectCount(),
                count);
            M_FAIL();
        }
        for (size_t i = 0; i < count; i++) {
            OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
            M_MUST(JSON_READ_A(io, i, &object->flags));
        }
        M_MUST(JSON_POP(io));
    }

    if (M_SHOULD(JSON_PUSH(io, "flyby_sequences"))) {
        const size_t count = JSON_ARRAY_LEN(io);
        const int32_t expected_count = Camera_GetSequenceCount();
        if (count != (size_t)expected_count) {
            JSON_ReadIO_SetError(
                io, "expected %d flyby sequences, got %d", expected_count,
                count);
            M_FAIL();
        }
        for (size_t i = 0; i < count; i++) {
            FLYBY_SEQUENCE *const sequence = Camera_GetSequence(i);
            M_MUST(JSON_READ_A(io, i, &sequence->one_shot));
        }
        M_MUST(JSON_POP(io));
    }

    M_FINISH();
}

bool SG_File_LoadLara(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "lara"));
    M_MUST(M_ReadLara(io));
    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadItems(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "items"));
    const int32_t count = JSON_ARRAY_LEN(io);

    Savegame_ProcessItemsBeforeLoad();

    for (int32_t i = 0; i < count; i++) {
        M_MUST(JSON_PUSH_INDEX(io, i));
        M_MUST(M_ReadItem(io, i));
        M_MUST(JSON_POP(io));
    }

    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadEffects(JSON_READ_IO *const io)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return true;
    }

    // Introduced in TRX 1.4
    if (!M_SHOULD(JSON_PUSH(io, "effects"))) {
        M_MUST(JSON_PUSH(io, "fx"));
    }
    for (int32_t i = 0;; i++) {
        if (!JSON_PUSH_INDEX(io, i)) {
            break;
        }
        if (i < MAX_EFFECTS) {
            M_ReadEffect(io);
        } else {
            LOG_WARNING(
                "Malformed save: expected a max of %d effect, got at least "
                "%d. Extra effects will be ignored.",
                MAX_EFFECTS - 1, i);
        }
        M_MUST(JSON_POP(io));
    }
    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadFX(JSON_READ_IO *const io)
{
    if (!M_OPTIONAL(JSON_PUSH(io, "vfx"))) {
        return true;
    }
    M_MUST(FX_Load(io));
    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadFlares(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "flares"));
    for (int32_t i = 0;; i++) {
        if (!JSON_PUSH_INDEX(io, i)) {
            break;
        }
        M_MUST(M_ReadFlare(io));
        M_MUST(JSON_POP(io));
    }
    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadMusic(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "music"));
    M_MUST(JSON_PUSH(io, "current"));
    M_MUST(M_ReadMusicTracks(io));
    M_MUST(JSON_POP(io));
    M_MUST(JSON_PUSH(io, "flags"));
    M_MUST(M_ReadMusicTrackFlags(io));
    M_MUST(JSON_POP(io));
    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadResumeInfoList(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "resume_info"));
    const int32_t length = JSON_ARRAY_LEN(io);
    const int32_t expected_length = GF_GetLevelTable(GFLT_MAIN)->count;
    if (length != expected_length) {
        JSON_ReadIO_SetError(
            io, "expected %d resume info elements, got %d", expected_length,
            length);
        M_FAIL();
    }
    for (int32_t i = 0; i < length; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        RESUME_INFO *const resume = SG_Resume_GetEntry(level);
        M_MUST(JSON_PUSH_INDEX(io, i));
        const bool has_prev_level = JSON_ReadIO_HasKey(io, "prev_level");
        M_MUST(M_ReadResumeInfo(io, resume));
        M_MUST(JSON_POP(io));

        // TRX 1.0/1.1 did not store prev_level for resume entries. Infer the
        // canonical predecessor so "Play previous levels" can carry loadout.
        if (!has_prev_level && resume->prev_level == -1) {
            const GF_LEVEL *const prev_level = GF_GetLevelBefore(level);
            if (prev_level != nullptr) {
                resume->prev_level = prev_level->num;
            }
        }
    }
    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadRules(JSON_READ_IO *const io)
{
    // Introduced in TRX 1.10, only carrying the rules that are off their
    // defaults. Keyed by name over the rules this build has, so a block that
    // names one it dropped, omits one it gained, or is absent entirely still
    // loads. What the save does not carry stays where SG_Resume_ResetAllEntries
    // left it.
    if (!M_OPTIONAL(JSON_PUSH(io, "rules"))) {
        return true;
    }

    const JSON_OBJECT *const rules = JSON_ReadIO_GetCurrentObject(io);
    for (const RULE *rule = Rules_GetMap(); rule->name != nullptr; rule++) {
        if (!JSON_ReadIO_HasKey(io, rule->name)) {
            continue;
        }
        TRX_VALUE value;
        M_MUST(JSONValue_Read(rules, rule->name, rule->type, nullptr, &value));
        const char *const err =
            Value_WritePtr(rule->type, rule->target, &value);
        if (err != nullptr) {
            LOG_WARNING("%s: %s", rule->name, err);
        }
    }

    M_MUST(JSON_POP(io));
    M_FINISH();
}

bool SG_File_LoadMisc(JSON_READ_IO *const io)
{
    M_MUST(JSON_PUSH(io, "misc"));

    {
        int32_t bonus_flag = false;
        M_MUST(JSON_READ(io, "bonus_flag", &bonus_flag));
        // saves made before Japanese mode was retired may carry its bit
        Game_SetBonusFlag(bonus_flag & GBF_NGPLUS);
    }

    {
        bool allies_hostile = false;
        M_MUST(JSON_READ(io, "are_monks_angry", &allies_hostile));
        Creature_SetAlliesHostile(allies_hostile);
    }

    {
        int32_t sunset_timer;
        M_MUST(JSON_READ(io, "sunset_timer", &sunset_timer));
        Output_SetTimeInGame(sunset_timer);
    }

    {
        // Introduced in TRX 1.4
        int32_t rng_control_seed = 0;
        if (M_OPTIONAL(JSON_READ(io, "rng_control_seed", &rng_control_seed))) {
            Random_SeedControl(rng_control_seed);
        }
    }

    {
        // Introduced in TRX 1.4
        int32_t rng_draw_seed = 0;
        if (M_OPTIONAL(JSON_READ(io, "rng_draw_seed", &rng_draw_seed))) {
            Random_SeedDraw(rng_draw_seed);
        }
    }

    {
        // Introduced in TRX 1.10
        uint64_t cutscenes_played = 0;
        if (M_OPTIONAL(JSON_READ(io, "cutscenes_played", &cutscenes_played))) {
            CutSeq_SetPlayedMask(cutscenes_played);
        }
    }

    {
        const GF_LEVEL *const current_level = Game_GetCurrentLevel();
        RESUME_INFO *const resume = SG_Resume_GetEntry(current_level);
        resume->stats.death_count = -1;
        M_MUST(JSON_READ(io, "death_count", &resume->stats.death_count));
    }

    {
        int32_t weather_type = (int32_t)WEATHER_NONE;
        if (M_OPTIONAL(JSON_READ(io, "weather_type", &weather_type))) {
            if (weather_type >= (int32_t)WEATHER_NONE
                && weather_type <= (int32_t)WEATHER_SNOW) {
                FX_Weather_SetWeather((WEATHER_TYPE)weather_type);
            } else {
                FX_Weather_SetWeather(WEATHER_NONE);
            }
        }
    }

    M_MUST(JSON_POP(io));
    M_FINISH();
}
