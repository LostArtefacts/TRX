#include <trx/game/items/actions/ids.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/rooms.h>

#include <lauxlib.h>

// Scripts count rooms from 1; the engine counts from 0. There is no
// Room_GetIndex, so derive it the way Item_GetIndex does.
static bool M_GetIndex(const void *const self, FIELD_VALUE *const out)
{
    const ROOM *const room = self;
    *out = (FIELD_VALUE) {
        .type = FT_INT16,
        .as_int = (int32_t)(room - Room_Get(0)) + 1,
    };
    return true;
}

// clang-format off
static const FIELD_DESC m_Fields[] = {
    FIELD_FN("room_index", FT_INT16, M_GetIndex, nullptr),

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

// A room is never recycled within a level, but the table is replaced whole at
// the next one. A bounds check alone would let a handle held across the change
// resolve to a different room; the generation is what makes it go stale.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    if (ref->gen != Room_GetGeneration()) {
        return nullptr;
    }
    return Room_Get(ref->idx);
}

static void M_PushRoom(lua_State *const L, const int32_t idx)
{
    LUA_Struct_Push(L, &TYPE_ROOM, M_Resolve, idx, Room_GetGeneration());
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
    // Read wide and range-test before narrowing, as find_valid_pos does: an
    // index past int32_t's width would wrap into range and name a room the
    // script did not ask for.
    const lua_Integer num = luaL_checkinteger(L, 1);
    if (num < 1 || num > Room_GetCount()) {
        lua_pushnil(L);
        return 1;
    }
    M_PushRoom(L, (int32_t)(num - 1));
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

// trxc.rooms.get_flipped_room(room) -> 1-based room number or nil
static int M_L_RoomsGetFlippedRoom(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ROOM);
    const ROOM *const room = LUA_Struct_Deref(L, ref);
    LUA_PushOptIndex(L, room->flipped_room, NO_ROOM);
    return 1;
}

// trxc.rooms.flip()
static int M_L_RoomsFlip(lua_State *const L)
{
    Room_FlipMap();
    return 0;
}

// trxc.rooms.flip_effect(effect_id, [timer])
static int M_L_RoomsFlipEffect(lua_State *const L)
{
    const int32_t trx_effect_id = luaL_checkinteger(L, 1);
    if (trx_effect_id == -1) {
        Room_SetFlipEffect(-1);
    } else {
        const ITEM_ACTION game_id = ItemAction_ToGameID(trx_effect_id);
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
        L, room_arg >= 1 && room_arg <= Room_GetCount(), 2, "unknown room");
    int16_t room_num = (int16_t)(room_arg - 1);
    if (!Room_FindValidPos(&pos, &room_num)) {
        lua_pushnil(L);
        return 1;
    }

    LUA_PushXYZ(L, pos);
    lua_pushinteger(L, room_num + 1);
    return 2;
}

static const luaL_Reg m_Module[] = {
    { "get_flipped_room", M_L_RoomsGetFlippedRoom },
    { "count", M_L_RoomsCount },
    { "get", M_L_RoomsGet },
    { "get_bounds", M_L_RoomsGetBounds },
    { "flip", M_L_RoomsFlip },
    { "flip_effect", M_L_RoomsFlipEffect },
    { "find_valid_pos", M_L_RoomsFindValidPos },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_ROOM, nullptr);
    LUA_RegisterModule(L, "rooms", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
