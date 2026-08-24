#include <trx/core/utils.h>
#include <trx/game/anims.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/items.h>
#include <trx/game/items/utils.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vars.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/rooms.h>

typedef struct {
    XYZ_32 min;
    XYZ_32 max;
} M_BOX;

typedef struct {
    XYZ_32 centre;
    int64_t radius_sq;
} M_SPHERE;

static bool M_GetAnim(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_S16,
        .as_int = Item_GetRelativeAnim(self),
    };
    return true;
}

static const char *M_SetAnim(void *const self, const TRX_VALUE *const in)
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

static bool M_GetFrame(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_S16,
        .as_int = Item_GetRelativeFrame(self),
    };
    return true;
}

static const char *M_SetFrame(void *const self, const TRX_VALUE *const in)
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

static bool M_GetIsAlive(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) { .type = TVT_BOOL, .as_bool = Item_IsAlive(self) };
    return true;
}

static bool M_GetIsTargetable(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) { .type = TVT_BOOL, .as_bool = Item_IsTargetable(self) };
    return true;
}

static bool M_GetIsKilled(const void *const self, TRX_VALUE *const out)
{
    const ITEM *const item = self;
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = item->is_destroyed,
    };
    return true;
}

static bool M_GetIsOneShot(const void *const self, TRX_VALUE *const out)
{
    const ITEM *const item = self;
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = item->trigger.spent,
    };
    return true;
}

static const char *M_SetIsOneShot(void *const self, const TRX_VALUE *const in)
{
    ITEM *const item = self;
    if (in->as_bool) {
        item->trigger.spent = true;
    } else {
        item->trigger.spent = false;
    }
    return nullptr;
}

static bool M_GetIsInPlay(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = Item_IsInPlay(self),
    };
    return true;
}

static bool M_GetIsHostile(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = Creature_IsHostile(self),
    };
    return true;
}

static bool M_GetIsAlly(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = Creature_IsAlly(self),
    };
    return true;
}

static bool M_GetIndex(const void *const self, TRX_VALUE *const out)
{
    // The index trx.items[i] takes; counts from 0, as the engine does.
    *out = (TRX_VALUE) {
        .type = TVT_S16,
        .as_int = Item_GetIndex(self),
    };
    return true;
}

static bool M_GetIsTriggered(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = Item_IsTriggerActiveRO(self),
    };
    return true;
}

static bool M_GetTriggerMask(const void *const self, TRX_VALUE *const out)
{
    *out = (TRX_VALUE) {
        .type = TVT_S32,
        .as_int = Item_GetTriggerMask(self),
    };
    return true;
}

static const char *M_SetTriggerMask(void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0 || in->as_int > TRIGGER_MASK_ALL) {
        return "trigger mask must be between 0 and 31";
    }
    Item_SetTriggerMask(self, in->as_int);
    return nullptr;
}

static bool M_GetIsReversed(const void *const self, TRX_VALUE *const out)
{
    const ITEM *const item = self;
    *out = (TRX_VALUE) {
        .type = TVT_BOOL,
        .as_bool = item->trigger.reversed,
    };
    return true;
}

static const char *M_SetIsReversed(void *const self, const TRX_VALUE *const in)
{
    ITEM *const item = self;
    item->trigger.reversed = in->as_bool;
    return nullptr;
}

static const char *M_SetPos(void *const self, const TRX_VALUE *const in)
{
    ITEM *const item = self;
    item->pos = in->as_xyz;
    Item_UpdateRoom(Item_GetIndex(item), Room_GetIndexFromPos(item->pos));
    return nullptr;
}

static const char *M_SetHitPoints(void *const self, const TRX_VALUE *const in)
{
    ITEM *const item = self;
    if (in->as_int > item->max_hit_points) {
        ObjectProperty_SetItemValueRaw(item, "max_hit_points", *in);
    }
    item->hit_points = in->as_int;
    return nullptr;
}

static const char *M_SetName(void *const self, const TRX_VALUE *const in)
{
    if (!Item_SetName(Item_GetIndex(self), in->as_str)) {
        return "item name already in use";
    }
    return nullptr;
}

// clang-format off
static const FIELD_DESC m_Fields[] = {
    // computed / validated. ITEM counts its animations and frames across the
    // whole level; these count within the object and within the animation, so
    // they take names of their own rather than shadowing the members.
    FIELD_FN("relative_anim_num",  TVT_S16,  M_GetAnim,         M_SetAnim),
    FIELD_FN("relative_frame_num", TVT_S16,  M_GetFrame,        M_SetFrame),
    FIELD_FN("item_num",           TVT_S16,  M_GetIndex,        nullptr),
    FIELD_FN("is_hostile",         TVT_BOOL, M_GetIsHostile,    nullptr),
    FIELD_FN("is_ally",            TVT_BOOL, M_GetIsAlly,       nullptr),
    FIELD_FN("is_alive",           TVT_BOOL, M_GetIsAlive,      nullptr),
    FIELD_FN("is_targetable",      TVT_BOOL, M_GetIsTargetable, nullptr),
    FIELD_FN("is_killed",          TVT_BOOL, M_GetIsKilled,     nullptr),
    FIELD_FN("is_one_shot",        TVT_BOOL, M_GetIsOneShot,    M_SetIsOneShot),

    // the trigger state a door reads before it acts and a creature ignores
    FIELD_FN("is_triggered", TVT_BOOL, M_GetIsTriggered, nullptr),
    FIELD_FN("trigger_mask", TVT_S32,  M_GetTriggerMask, M_SetTriggerMask),
    FIELD_FN("is_reversed",  TVT_BOOL, M_GetIsReversed,  M_SetIsReversed),

    // side-effecting writes
    FIELD_SET(ITEM, pos,        M_SetPos),
    FIELD_SET(ITEM, hit_points, M_SetHitPoints),
    FIELD_SET(ITEM, name,       M_SetName),

    // plain members
    FIELD_MODULAR(ITEM, rot),
    FIELD(ITEM, timer),
    FIELD(ITEM, is_visible),
    FIELD(ITEM, is_finished),
    FIELD(ITEM, speed),
    FIELD(ITEM, fall_speed),
    FIELD(ITEM, gravity),
    FIELD(ITEM, is_collidable),
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
    FIELD_RO(ITEM, is_simulated),
    FIELD_RO(ITEM, is_present),
    FIELD_FN("is_in_play", TVT_BOOL, M_GetIsInPlay, nullptr),
    FIELD_RO(ITEM, hit_status),
    FIELD_RO(ITEM, floor),
    FIELD_RO(ITEM, room_num),
    FIELD_RO(ITEM, anim_num),
    FIELD_RO(ITEM, frame_num),
    FIELD_RO(ITEM, prev_frame_num),
    FIELD_RO(ITEM, next_item),
    FIELD_RO(ITEM, next_simulated),
};
// clang-format on

TYPE_DEFINE(ITEM, m_Fields)

// The generation carried by the handle is what keeps an index from silently
// rebinding to the item that recycled the slot; Item_FromHandle checks it.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return Item_FromHandle(ref->handle);
}

static void M_PushItem(lua_State *const L, const int16_t idx)
{
    LUA_Struct_Push(L, &TYPE_ITEM, M_Resolve, Item_GetHandle(idx));
}

// The object property overlay stays a separate namespace: fields address the
// ITEM struct, properties are the object's declared defaults plus this item's
// own overrides. The bridges themselves are in lua/utils.
static bool M_GetPropertyValue(
    const void *const self, const char *const name, TRX_VALUE *const out)
{
    return ObjectProperty_GetItemValue(self, name, out);
}

static const char *M_SetPropertyValue(
    void *const self, const char *const name, const TRX_VALUE value)
{
    return ObjectProperty_SetItemValueRaw(self, name, value);
}

static int32_t M_GetPropertyCount(const void *const self)
{
    return ObjectProperty_GetItemNameCount(self);
}

static const char *M_GetPropertyName(const void *const self, const int32_t idx)
{
    return ObjectProperty_GetItemName(self, idx);
}

static const LUA_PROPERTY_DESC m_Properties = {
    .type = &TYPE_ITEM,
    .what = "item",
    .get = M_GetPropertyValue,
    .set = M_SetPropertyValue,
    .name_count = M_GetPropertyCount,
    .name_at = M_GetPropertyName,
};

// item:destroy()
static int M_L_ItemsDestroy(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Item_Destroy(ref->handle.id);
    return 0;
}

// item:activate()
static int M_L_ItemsActivate(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Item_Activate(ref->handle.id, true);
    return 0;
}

// item:deactivate()
static int M_L_ItemsDeactivate(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Item_Deactivate(ref->handle.id);
    return 0;
}

// An optional flag in an optional options table.
static bool M_OptFlag(
    lua_State *const L, const int32_t idx, const char *const key)
{
    if (!lua_istable(L, idx)) {
        return false;
    }
    lua_getfield(L, idx, key);
    const bool value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}

// An optional integer in an optional options table.
static int32_t M_OptInt(
    lua_State *const L, const int32_t idx, const char *const key,
    const int32_t fallback)
{
    if (!lua_istable(L, idx)) {
        return fallback;
    }
    lua_getfield(L, idx, key);
    const int32_t value =
        lua_isnil(L, -1) ? fallback : (int32_t)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return value;
}

// item:trigger([opts])
static int M_L_ItemsTrigger(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);

    const int32_t kind = M_OptInt(L, 2, "type", ITEM_TRIGGER_NORMAL);
    if (kind < 0 || kind > ITEM_TRIGGER_ANTI) {
        return luaL_error(L, "unknown trigger type %d", kind);
    }

    const int32_t mask = M_OptInt(L, 2, "mask", TRIGGER_MASK_ALL);
    if (mask < 0 || mask > TRIGGER_MASK_ALL) {
        return luaL_error(
            L, "trigger mask must be between 0 and %d, got %d",
            TRIGGER_MASK_ALL, mask);
    }

    float timer = 0.0f;
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "timer");
        if (!lua_isnil(L, -1)) {
            timer = (float)luaL_checknumber(L, -1);
        }
        lua_pop(L, 1);
    }

    const ITEM_TRIGGER trigger = {
        .kind = (ITEM_TRIGGER_KIND)kind,
        .mask = (int16_t)(mask & TRIGGER_MASK_ALL),
        .timer = timer,
        .one_shot = M_OptFlag(L, 2, "one_shot"),
    };
    Item_Trigger(ref->handle.id, &trigger);
    return 0;
}

// trxc.items.count() -> int
static int M_L_ItemsCount(lua_State *const L)
{
    lua_pushinteger(L, Item_GetTotalCount());
    return 1;
}

// Every item whose position passes the test, as a list of item numbers. The
// position is all this looks at; whether an item is in the world, simulated or
// alive is for the caller to narrow by.
static int M_PushItemsWhere(
    lua_State *const L, bool (*const test)(XYZ_32 pos, const void *arg),
    const void *const arg)
{
    lua_newtable(L);
    int32_t found = 0;
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        if (!test(Item_Get(i)->pos, arg)) {
            continue;
        }
        lua_pushinteger(L, i);
        lua_rawseti(L, -2, ++found);
    }
    return 1;
}

static bool M_InBox(const XYZ_32 pos, const void *const arg)
{
    const M_BOX *const box = arg;
    return pos.x >= box->min.x && pos.x <= box->max.x && pos.y >= box->min.y
        && pos.y <= box->max.y && pos.z >= box->min.z && pos.z <= box->max.z;
}

static bool M_InSphere(const XYZ_32 pos, const void *const arg)
{
    const M_SPHERE *const sphere = arg;
    const int64_t dx = pos.x - sphere->centre.x;
    const int64_t dy = pos.y - sphere->centre.y;
    const int64_t dz = pos.z - sphere->centre.z;
    return dx * dx + dy * dy + dz * dz <= sphere->radius_sq;
}

// trxc.items.in_box({x,y,z}, {x,y,z}) -> list of item numbers
static int M_L_ItemsInBox(lua_State *const L)
{
    const XYZ_32 a = LUA_CheckXYZ(L, 1);
    const XYZ_32 b = LUA_CheckXYZ(L, 2);
    const M_BOX box = {
        .min = { .x = MIN(a.x, b.x), .y = MIN(a.y, b.y), .z = MIN(a.z, b.z) },
        .max = { .x = MAX(a.x, b.x), .y = MAX(a.y, b.y), .z = MAX(a.z, b.z) },
    };
    return M_PushItemsWhere(L, M_InBox, &box);
}

// trxc.items.in_sphere({x,y,z}, radius) -> list of item numbers
static int M_L_ItemsInSphere(lua_State *const L)
{
    const XYZ_32 centre = LUA_CheckXYZ(L, 1);
    const int64_t radius = luaL_checkinteger(L, 2);
    if (radius < 0) {
        return luaL_argerror(L, 2, "radius must not be negative");
    }
    const M_SPHERE sphere = {
        .centre = centre,
        .radius_sq = radius * radius,
    };
    return M_PushItemsWhere(L, M_InSphere, &sphere);
}

// trxc.items.get(index | name) -> Item or nil
static int M_L_ItemsGet(lua_State *const L)
{
    int32_t idx = -1;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        int32_t num;
        if (!LUA_CheckBoundedInt(L, 1, 0, Item_GetTotalCount() - 1, &num)) {
            lua_pushnil(L);
            return 1;
        }
        idx = num;
    } else {
        const ITEM *const item = Item_GetByName(luaL_checkstring(L, 1));
        idx = item != nullptr ? Item_GetIndex(item) : -1;
    }
    if (idx < 0 || idx >= Item_GetTotalCount()) {
        lua_pushnil(L);
        return 1;
    }
    M_PushItem(L, idx);
    return 1;
}

// trxc.items.spawn(object_id, {x,y,z}, angle_y) -> Item or nil
static int M_L_ItemsSpawn(lua_State *const L)
{
    // Object_Get asserts on an id outside the table.
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const OBJECT *const obj = Object_Get(object_id);
    if (!obj->loaded) {
        return luaL_error(L, "object %d is not loaded", object_id);
    }

    const XYZ_32 pos = LUA_CheckXYZ(L, 2);
    const int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num == NO_ROOM) {
        return luaL_error(L, "position is outside the level");
    }

    // Read every argument that can raise before taking a slot: luaL_optinteger
    // longjmps on a non-numeric argument, and doing it after Item_Create would
    // leave a half-initialised item occupying the pool.
    const lua_Integer rot_y = luaL_optinteger(L, 3, 0);

    const int16_t idx = Item_Create();
    if (idx == NO_ITEM) {
        lua_pushnil(L);
        return 1;
    }

    ITEM *const item = Item_Get(idx);
    item->object_id = object_id;
    item->pos = pos;
    item->rot.y = rot_y;
    item->room_num = room_num;
    item->shade.value_1 = -1;
    Item_Initialise(idx);

    if (lua_istable(L, 4)) {
        lua_getfield(L, 4, "activate");
        const bool activate = lua_toboolean(L, -1);
        lua_pop(L, 1);
        if (activate) {
            Item_Activate(idx, true);
        }
    }

    M_PushItem(L, idx);
    return 1;
}

// item:distance_to({x,y,z}) -> integer
static int M_L_ItemsDistanceTo(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    const ITEM *const item = LUA_Struct_Deref(L, ref);
    lua_pushinteger(L, Item_GetDistance(item, LUA_CheckXYZ(L, 2)));
    return 1;
}

// trxc.items.get_bounds(item) -> { min_x=, min_y=, min_z=, max_x=, max_y=,
// max_z= }
//
// A table, not a scalar, so it cannot be a reflected field. Lua wraps this as a
// computed property.
static int M_L_ItemsGetBounds(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    const ITEM *const item = LUA_Struct_Deref(L, ref);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

    lua_newtable(L);
    lua_pushinteger(L, bounds->min.x);
    lua_setfield(L, -2, "min_x");
    lua_pushinteger(L, bounds->min.y);
    lua_setfield(L, -2, "min_y");
    lua_pushinteger(L, bounds->min.z);
    lua_setfield(L, -2, "min_z");
    lua_pushinteger(L, bounds->max.x);
    lua_setfield(L, -2, "max_x");
    lua_pushinteger(L, bounds->max.y);
    lua_setfield(L, -2, "max_y");
    lua_pushinteger(L, bounds->max.z);
    lua_setfield(L, -2, "max_z");
    return 1;
}

// item:die([explode])
static int M_L_ItemsDie(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Creature_Die(ref->handle.id, lua_toboolean(L, 2));
    return 0;
}

// item:take_damage(damage)
static int M_L_ItemsTakeDamage(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    ITEM *const item = LUA_Struct_Deref(L, ref);
    const lua_Integer damage = luaL_checkinteger(L, 2);
    luaL_argcheck(
        L, damage >= 0 && damage <= INT16_MAX, 2, "damage out of range");
    Item_TakeDamage(item, (int16_t)damage, IDF_NONE, nullptr);
    return 0;
}

// item:shatter([damage])
static int M_L_ItemsShatter(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Item_Shatter(ref->handle.id, -1, (int16_t)luaL_optinteger(L, 2, 0));
    return 0;
}

static const luaL_Reg m_Methods[] = {
    { "distance_to", M_L_ItemsDistanceTo },
    { "die", M_L_ItemsDie },
    { "take_damage", M_L_ItemsTakeDamage },
    { "shatter", M_L_ItemsShatter },
    { "destroy", M_L_ItemsDestroy },
    { "activate", M_L_ItemsActivate },
    { "deactivate", M_L_ItemsDeactivate },
    { "trigger", M_L_ItemsTrigger },
    { nullptr, nullptr },
};

static const luaL_Reg m_Module[] = {
    { "count", M_L_ItemsCount },
    { "get", M_L_ItemsGet },
    { "get_bounds", M_L_ItemsGetBounds },
    { "in_box", M_L_ItemsInBox },
    { "in_sphere", M_L_ItemsInSphere },
    { "spawn", M_L_ItemsSpawn },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_ITEM, m_Methods);
    LUA_Property_Register(L, &m_Properties);

    LUA_RegisterModule(L, "items", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
