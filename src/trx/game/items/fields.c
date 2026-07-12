#include <trx/core/field.h>
#include <trx/game/anims.h>
#include <trx/game/creature.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>

static bool M_GetAnim(const void *const self, FIELD_VALUE *const out)
{
    *out = (FIELD_VALUE) {
        .type = FT_INT16,
        .as_int = Item_GetRelativeAnim(self),
    };
    return true;
}

static const char *M_SetAnim(void *const self, const FIELD_VALUE *const in)
{
    ITEM *const item = self;
    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->anim_idx == NO_ANIM) {
        return "object has no animations";
    }
    if (in->as_int < 0 || in->as_int >= Anim_GetTotalCount()
        || in->as_int >= obj->anim_count) {
        return "invalid animation index";
    }
    const ANIM *const anim = Anim_GetAnim(obj->anim_idx + in->as_int);
    if (anim->frame_ptr == nullptr) {
        return "invalid animation index";
    }
    item->anim_num = obj->anim_idx + in->as_int;
    item->frame_num = anim->frame_base;
    return nullptr;
}

static bool M_GetFrame(const void *const self, FIELD_VALUE *const out)
{
    *out = (FIELD_VALUE) {
        .type = FT_INT16,
        .as_int = Item_GetRelativeFrame(self),
    };
    return true;
}

static const char *M_SetFrame(void *const self, const FIELD_VALUE *const in)
{
    ITEM *const item = self;
    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->anim_idx == NO_ANIM) {
        return "object has no animations";
    }
    const ANIM *const anim = Item_GetAnim(item);
    if (in->as_int < 0) {
        if (anim->frame_end + in->as_int + 1 < anim->frame_base) {
            return "invalid frame index";
        }
        item->frame_num = anim->frame_end + in->as_int + 1;
    } else {
        if (anim->frame_base + in->as_int >= anim->frame_end) {
            return "invalid frame index";
        }
        item->frame_num = anim->frame_base + in->as_int;
    }
    return nullptr;
}

// Scripts count rooms from 1; the engine counts from 0.
static bool M_GetRoomIndex(const void *const self, FIELD_VALUE *const out)
{
    const ITEM *const item = self;
    *out = (FIELD_VALUE) { .type = FT_INT16, .as_int = item->room_num + 1 };
    return true;
}

static bool M_GetIsAlive(const void *const self, FIELD_VALUE *const out)
{
    *out = (FIELD_VALUE) { .type = FT_BOOL, .as_bool = Item_IsAlive(self) };
    return true;
}

static bool M_GetIsKilled(const void *const self, FIELD_VALUE *const out)
{
    const ITEM *const item = self;
    *out = (FIELD_VALUE) {
        .type = FT_BOOL,
        .as_bool = (item->flags & IF_KILLED) != 0,
    };
    return true;
}

static bool M_GetIsHostile(const void *const self, FIELD_VALUE *const out)
{
    *out = (FIELD_VALUE) {
        .type = FT_BOOL,
        .as_bool = Creature_IsHostile(self),
    };
    return true;
}

static const char *M_SetPos(void *const self, const FIELD_VALUE *const in)
{
    ITEM *const item = self;
    item->pos = in->as_xyz;
    Item_UpdateRoom(Item_GetIndex(item), Room_GetIndexFromPos(item->pos));
    return nullptr;
}

static const char *M_SetHitPoints(void *const self, const FIELD_VALUE *const in)
{
    ITEM *const item = self;
    item->hit_points = in->as_int;
    if (item->hit_points > item->max_hit_points) {
        ObjectProperty_SetItemValueRaw(
            item, "max_hit_points",
            (OBJECT_PROPERTY_VALUE) {
                .type = OBJECT_PROPERTY_TYPE_INT,
                .as_int = item->hit_points,
            });
        item->max_hit_points = item->hit_points;
    }
    return nullptr;
}

static const char *M_SetName(void *const self, const FIELD_VALUE *const in)
{
    if (!Item_SetName(Item_GetIndex(self), in->as_str)) {
        return "item name already in use";
    }
    return nullptr;
}

// clang-format off
static const FIELD_DESC M_ITEM_FIELDS[] = {
    // computed / validated
    FIELD_FN("anim",       FT_INT16, M_GetAnim,      M_SetAnim),
    FIELD_FN("frame",      FT_INT16, M_GetFrame,     M_SetFrame),
    FIELD_FN("room_index", FT_INT16, M_GetRoomIndex, nullptr),
    FIELD_FN("is_hostile", FT_BOOL,  M_GetIsHostile, nullptr),
    FIELD_FN("is_alive",   FT_BOOL,  M_GetIsAlive,   nullptr),
    FIELD_FN("is_killed",  FT_BOOL,  M_GetIsKilled,  nullptr),

    // side-effecting writes
    FIELD_SET(ITEM, pos,        M_SetPos),
    FIELD_SET(ITEM, hit_points, M_SetHitPoints),
    FIELD_SET(ITEM, name,       M_SetName),

    // plain members
    FIELD(ITEM, rot),
    FIELD(ITEM, timer),
    FIELD(ITEM, flags),
    FIELD(ITEM, status),
    FIELD(ITEM, speed),
    FIELD(ITEM, fall_speed),
    FIELD(ITEM, gravity),
    FIELD(ITEM, collidable),
    FIELD(ITEM, enable_shadow),
    FIELD(ITEM, enable_interpolation),
    FIELD(ITEM, dynamic_light),
    FIELD(ITEM, looked_at),
    FIELD(ITEM, clear_body),
    FIELD(ITEM, include_in_kill_stats),
    FIELD(ITEM, mesh_bits),
    FIELD(ITEM, touch_bits),
    FIELD(ITEM, ai_bits),
    FIELD(ITEM, ai_tag),
    FIELD(ITEM, after_death),
    FIELD(ITEM, box_num),
    FIELD(ITEM, current_anim_state),
    FIELD(ITEM, goal_anim_state),
    FIELD(ITEM, required_anim_state),

    // raw engine state - reachable by C, but the write paths are unsafe or the
    // semantics are internal, so these are read-only here
    FIELD_RO(ITEM, object_id),
    FIELD_RO(ITEM, max_hit_points),
    FIELD_RO(ITEM, active),
    FIELD_RO(ITEM, hit_status),
    FIELD_RO(ITEM, floor),
    FIELD_RO(ITEM, room_num),
    FIELD_RO(ITEM, anim_num),
    FIELD_RO(ITEM, frame_num),
    FIELD_RO(ITEM, prev_frame_num),
    FIELD_RO(ITEM, next_item),
    FIELD_RO(ITEM, next_active),
    FIELD_RO(ITEM, gen),
};
// clang-format on

TYPE_DEFINE(ITEM, M_ITEM_FIELDS)
