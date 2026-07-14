#include <trx/game/items/actions/ids.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/rooms.h>

#include <lauxlib.h>

extern const TYPE_DESC TYPE_ROOM;

// Rooms are a fixed array for the level's lifetime: unlike items they are never
// recycled, so the handle needs no generation - only a bounds check.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    if (ref->idx < 0 || ref->idx >= Room_GetCount()) {
        return nullptr;
    }
    return Room_Get(ref->idx);
}

static void M_PushRoom(lua_State *const L, const int32_t idx)
{
    LUA_Struct_Push(L, &TYPE_ROOM, M_Resolve, idx, 0);
}

// trxc.rooms.count() -> int
static int M_L_Count(lua_State *const L)
{
    lua_pushinteger(L, Room_GetCount());
    return 1;
}

// trxc.rooms.get(index) -> Room or nil
static int M_L_Get(lua_State *const L)
{
    const int32_t idx = luaL_checkinteger(L, 1) - 1;
    if (idx < 0 || idx >= Room_GetCount()) {
        lua_pushnil(L);
        return 1;
    }
    M_PushRoom(L, idx);
    return 1;
}

// trxc.rooms.get_bounds(room) -> { min_x=, min_y=, min_z=, max_x=, max_y=,
// max_z= }
//
// A table, not a scalar, so it cannot be a reflected field. Lua wraps this as a
// computed property.
static int M_L_GetBounds(lua_State *const L)
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
//
// The public API turns this into a Room object, so it stays a helper rather
// than a reflected field.
static int M_L_GetFlippedRoom(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ROOM);
    const ROOM *const room = LUA_Struct_Deref(L, ref);
    if (room->flipped_room == NO_ROOM) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, room->flipped_room + 1);
    }
    return 1;
}

// trxc.rooms.flip()
static int M_L_Flip(lua_State *const L)
{
    Room_FlipMap();
    return 0;
}

// trxc.rooms.flip_effect(effect_id, [timer])
static int M_L_FlipEffect(lua_State *const L)
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
static int M_L_FindValidPos(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    XYZ_32 pos = {};
    lua_getfield(L, 1, "x");
    pos.x = luaL_checkinteger(L, -1);
    lua_getfield(L, 1, "y");
    pos.y = luaL_checkinteger(L, -1);
    lua_getfield(L, 1, "z");
    pos.z = luaL_checkinteger(L, -1);
    lua_pop(L, 3);

    int16_t room_num = luaL_checkinteger(L, 2) - 1;
    if (!Room_FindValidPos(&pos, &room_num)) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    lua_pushinteger(L, pos.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, pos.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, pos.z);
    lua_setfield(L, -2, "z");
    lua_pushinteger(L, room_num + 1);
    return 2;
}

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_ROOM, nullptr);

    lua_getglobal(L, "trxc");
    lua_newtable(L);

    lua_pushcfunction(L, M_L_GetFlippedRoom);
    lua_setfield(L, -2, "get_flipped_room");
    lua_pushcfunction(L, M_L_Count);
    lua_setfield(L, -2, "count");
    lua_pushcfunction(L, M_L_Get);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, M_L_GetBounds);
    lua_setfield(L, -2, "get_bounds");
    lua_pushcfunction(L, M_L_Flip);
    lua_setfield(L, -2, "flip");
    lua_pushcfunction(L, M_L_FlipEffect);
    lua_setfield(L, -2, "flip_effect");
    lua_pushcfunction(L, M_L_FindValidPos);
    lua_setfield(L, -2, "find_valid_pos");
    lua_setfield(L, -2, "rooms");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
