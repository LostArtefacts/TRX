#include <trx/core/vector.h>
#include <trx/game/console/common.h>
#include <trx/game/items/actions/ids.h>
#include <trx/game/items/const.h>
#include <trx/game/level/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/rooms.h>

#include <lauxlib.h>

typedef struct {
    int32_t room_num;
    int32_t group;
} M_FLIP_GROUP_DECL;

static VECTOR *m_FlipGroupDecls = nullptr;

// Scripts and the engine both count rooms from 0. There is no Room_GetIndex, so
// derive it the way Item_GetIndex does.
static bool M_GetIndex(const void *const self, TRX_VALUE *const out)
{
    const ROOM *const room = self;
    *out = (TRX_VALUE) {
        .type = TVT_S16,
        .as_int = (int32_t)(room - Room_Get(0)),
    };
    return true;
}

// clang-format off
static const FIELD_DESC m_Fields[] = {
    FIELD_FN("room_num", TVT_S16, M_GetIndex, nullptr),

    FIELD(ROOM, flags.underwater),
    FIELD(ROOM, flags.wind),
    FIELD(ROOM, flags.damaging),
    FIELD(ROOM, flags.cold),
    FIELD(ROOM, flags.outside),
    FIELD(ROOM, flags.inside),
    FIELD(ROOM, flags.swamp),
    FIELD(ROOM, flags.dynamic_lit),
    FIELD(ROOM, flags.no_lens_flare),

    FIELD_RO(ROOM, flip_status),
    FIELD_RO(ROOM, flipped_room),
    FIELD_RO(ROOM, pos),
    FIELD_RO(ROOM, min_floor),
    FIELD_RO(ROOM, max_ceiling),
    FIELD_RO(ROOM, size.x),
    FIELD_RO(ROOM, size.z),
    FIELD_RO(ROOM, ambient),
    FIELD_RO(ROOM, num_lights),
    FIELD_RO(ROOM, num_static_meshes),
    FIELD_RO(ROOM, item_num),
    FIELD_RO(ROOM, effect_num),
    FIELD_RO(ROOM, water_scheme),
    FIELD_RO(ROOM, reverb_info),
    FIELD_RO(ROOM, alternate_group),
};
// clang-format on

TYPE_DEFINE(ROOM, m_Fields)

// A bounds check alone would let a handle held across a level change resolve to
// a different room; Room_FromHandle checks the epoch that makes it go stale.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return Room_FromHandle(ref->handle);
}

static void M_PushRoom(lua_State *const L, const int32_t idx)
{
    LUA_Struct_Push(L, &TYPE_ROOM, M_Resolve, Room_GetHandle(idx));
}

// trxc.rooms.count() -> int
static int M_L_RoomsCount(lua_State *const L)
{
    lua_pushinteger(L, Room_GetCount());
    return 1;
}

// trxc.rooms.get(index) -> Room or nil
static int M_L_RoomsGet(lua_State *const L)
{
    int32_t num;
    if (!LUA_CheckBoundedInt(L, 1, 0, Room_GetCount() - 1, &num)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushRoom(L, num);
    return 1;
}

// trxc.rooms.get_bounds(room) -> { min_x=, min_y=, min_z=, max_x=, max_y=,
// max_z= }
//
// A table, not a scalar, so it cannot be a reflected field. Lua wraps this as a
// computed property.
static int M_L_RoomsGetBounds(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ROOM);
    const ROOM *const room = LUA_Struct_Deref(L, ref);
    const BOUNDS_32 bounds = Room_GetRoomBounds(room);

    lua_newtable(L);
    lua_pushinteger(L, bounds.min.x);
    lua_setfield(L, -2, "min_x");
    lua_pushinteger(L, bounds.min.y);
    lua_setfield(L, -2, "min_y");
    lua_pushinteger(L, bounds.min.z);
    lua_setfield(L, -2, "min_z");
    lua_pushinteger(L, bounds.max.x);
    lua_setfield(L, -2, "max_x");
    lua_pushinteger(L, bounds.max.y);
    lua_setfield(L, -2, "max_y");
    lua_pushinteger(L, bounds.max.z);
    lua_setfield(L, -2, "max_z");
    return 1;
}

// trxc.rooms.point_inside(room, {x,y,z}) -> bool
static int M_L_RoomsPointInside(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ROOM);
    const ROOM *const room = LUA_Struct_Deref(L, ref);
    lua_pushboolean(L, Room_PointInside(room, LUA_CheckXYZ(L, 2)));
    return 1;
}

// trxc.rooms.get_flipped_room(room) -> 0-based room number or nil
static int M_L_RoomsGetFlippedRoom(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ROOM);
    const ROOM *const room = LUA_Struct_Deref(L, ref);
    LUA_PushOptIndex(L, room->flipped_room, NO_ROOM);
    return 1;
}

// trxc.rooms.flip_group_count() -> integer
static int M_L_RoomsFlipGroupCount(lua_State *const L)
{
    lua_pushinteger(L, MAX_FLIP_MAPS);
    return 1;
}

// trxc.rooms.declare_flip_group(room_num, group)
//
// Stores flip groups until Level_Initialise, when rooms are ready.
// Keeps declarations level-scoped, like the listeners from the same script.
static int M_L_RoomsDeclareFlipGroup(lua_State *const L)
{
    if (LUA_GetScriptContext() != LUA_CONTEXT_LEVEL) {
        return luaL_error(
            L, "a flip group can only be declared by a level script");
    }
    if (Level_IsWorldLoaded()) {
        return luaL_error(
            L, "a flip group must be declared before the level is read");
    }

    const lua_Integer room_num = luaL_checkinteger(L, 1);
    luaL_argcheck(L, room_num >= 0, 1, "unknown room");
    const lua_Integer group = luaL_checkinteger(L, 2);
    luaL_argcheck(
        L, group >= 0 && group < MAX_FLIP_MAPS, 2, "no such flip group");

    if (m_FlipGroupDecls == nullptr) {
        m_FlipGroupDecls = Vector_Create(sizeof(M_FLIP_GROUP_DECL));
    }
    const M_FLIP_GROUP_DECL decl = {
        .room_num = (int32_t)room_num,
        .group = (int32_t)group,
    };
    Vector_Add(m_FlipGroupDecls, &decl);
    return 0;
}

// trxc.rooms.flip([group])
static int M_L_RoomsFlip(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        for (int32_t group = 0; group < MAX_FLIP_MAPS; group++) {
            Room_FlipMap(group);
        }
        return 0;
    }
    const lua_Integer group = luaL_checkinteger(L, 1);
    if (group < 0 || group >= MAX_FLIP_MAPS) {
        return luaL_error(L, "no such flip group");
    }
    Room_FlipMap((int32_t)group);
    return 0;
}

// trxc.rooms.get_flipped([group]) -> bool
static int M_L_RoomsGetFlipped(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        lua_pushboolean(L, Room_GetFlipStatus());
        return 1;
    }
    const lua_Integer group = luaL_checkinteger(L, 1);
    if (group < 0 || group >= MAX_FLIP_MAPS) {
        return luaL_error(L, "no such flip group");
    }
    lua_pushboolean(L, Room_GetFlipGroupStatus((int32_t)group));
    return 1;
}

// trxc.rooms.flip_effect(effect_id, [timer])
static int M_L_RoomsFlipEffect(lua_State *const L)
{
    const int32_t trx_effect_id = luaL_checkinteger(L, 1);
    if (trx_effect_id == -1) {
        Room_SetFlipEffect(-1);
    } else {
        const ITEM_ACTION_SLOT game_id = ItemAction_IDToSlot(trx_effect_id);
        if (game_id == ITEM_ACTION_INVALID) {
            return luaL_error(L, "invalid flip effect id");
        }
        Room_SetFlipEffect(game_id);
    }

    if (lua_gettop(L) >= 2) {
        Room_SetFlipTimer(luaL_checkinteger(L, 2));
    }
    return 0;
}

// trxc.rooms.find_valid_pos({x,y,z}, room_num) -> pos, room_num or nil
static int M_L_RoomsFindValidPos(lua_State *const L)
{
    XYZ_32 pos = LUA_CheckXYZ(L, 1);

    // Room_FindValidPos dereferences Room_Get(room_num) without checking, and
    // Room_Get gives back nullptr for a room outside the level.
    const lua_Integer room_arg = luaL_checkinteger(L, 2);
    luaL_argcheck(
        L, room_arg >= 0 && room_arg <= Room_GetCount() - 1, 2, "unknown room");
    int16_t room_num = (int16_t)room_arg;
    if (!Room_FindValidPos(&pos, &room_num)) {
        lua_pushnil(L);
        return 1;
    }

    LUA_PushXYZ(L, pos);
    lua_pushinteger(L, room_num);
    return 2;
}

// trxc.rooms.get_height({x,y,z}, room_num, opts) -> floor height, or nil where
// there is no floor
static int M_L_RoomsGetHeight(lua_State *const L)
{
    const XYZ_32 pos = LUA_CheckXYZ(L, 1);

    int16_t room_num;
    if (lua_isnoneornil(L, 2)) {
        room_num = Room_GetIndexFromPos(pos);
        if (room_num == NO_ROOM) {
            lua_pushnil(L);
            return 1;
        }
    } else {
        // Room_GetSector walks from the room it is given, and Room_Get gives
        // back nullptr for a room outside the level.
        const lua_Integer room_arg = luaL_checkinteger(L, 2);
        luaL_argcheck(
            L, room_arg >= 0 && room_arg <= Room_GetCount() - 1, 2,
            "unknown room");
        room_num = (int16_t)room_arg;
    }

    bool fix_tilts = true;
    if (lua_istable(L, 3)) {
        lua_getfield(L, 3, "fix_tilts");
        if (!lua_isnil(L, -1)) {
            fix_tilts = lua_toboolean(L, -1);
        }
        lua_pop(L, 1);
    }

    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeightEx(sector, pos, fix_tilts, NO_ITEM);
    if (height == NO_HEIGHT) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, height);
    }
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "get_flipped_room", M_L_RoomsGetFlippedRoom },
    { "get_height", M_L_RoomsGetHeight },
    { "count", M_L_RoomsCount },
    { "get", M_L_RoomsGet },
    { "get_bounds", M_L_RoomsGetBounds },
    { "point_inside", M_L_RoomsPointInside },
    { "declare_flip_group", M_L_RoomsDeclareFlipGroup },
    { "flip", M_L_RoomsFlip },
    { "flip_group_count", M_L_RoomsFlipGroupCount },
    { "get_flipped", M_L_RoomsGetFlipped },
    { "flip_effect", M_L_RoomsFlipEffect },
    { "find_valid_pos", M_L_RoomsFindValidPos },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_ROOM, nullptr);
    LUA_RegisterModule(L, "rooms", m_Module);
}

void LUA_Rooms_ClearFlipGroups(void)
{
    if (m_FlipGroupDecls != nullptr) {
        Vector_Clear(m_FlipGroupDecls);
    }
}

void LUA_Rooms_ApplyFlipGroups(void)
{
    if (m_FlipGroupDecls == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_FlipGroupDecls->count; i++) {
        const M_FLIP_GROUP_DECL *const decl = Vector_Get(m_FlipGroupDecls, i);
        if (decl->room_num >= Room_GetCount()) {
            Console_ShowError(
                "Lua flip group error: unknown room %d", decl->room_num);
            continue;
        }
        if (Room_Get(decl->room_num)->flipped_room == NO_ROOM) {
            Console_ShowError(
                "Lua flip group error: room %d has no flip pair",
                decl->room_num);
            continue;
        }
        Room_SetFlipGroup(decl->room_num, decl->group);
    }
}

REGISTER_LUA_CAPI(.create = M_Create)
