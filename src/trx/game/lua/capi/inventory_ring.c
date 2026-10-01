#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/inventory_ring/types.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/common.h>

#include <lauxlib.h>

// trxc.inventory_ring.icon_of(object) -> object id or nil
static int M_L_RingIconOf(lua_State *const L)
{
    const OBJECT_ID icon_id = Inv_GetItemOption(LUA_CheckObjectID(L, 1));
    if (icon_id == NO_OBJECT) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, icon_id);
    }
    return 1;
}

static int32_t M_ReadInt(
    lua_State *const L, const int idx, const char *const key,
    const int32_t fallback)
{
    lua_getfield(L, idx, key);
    const int32_t value =
        lua_isnil(L, -1) ? fallback : (int32_t)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return value;
}

static float M_ReadNum(
    lua_State *const L, const int idx, const char *const key,
    const float fallback)
{
    lua_getfield(L, idx, key);
    const float value =
        lua_isnil(L, -1) ? fallback : (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);
    return value;
}

static INVENTORY_ITEM *M_FindItem(const OBJECT_ID object_id)
{
    for (int32_t i = 0; i < g_InvRing_Items->count; i++) {
        INVENTORY_ITEM *const item =
            *(INVENTORY_ITEM **)Vector_Get(g_InvRing_Items, i);
        if (item->object_id == object_id) {
            return item;
        }
    }
    return nullptr;
}

// trxc.inventory_ring.declare_item(spec)
static int M_L_RingDeclareItem(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "object_id");
    const char *const name = luaL_checkstring(L, -1);
    const OBJECT_ID object_id = Object_IdFromKey(name);
    if (object_id == NO_OBJECT) {
        luaL_error(L, "unknown object '%s'", name);
    }
    lua_pop(L, 1);

    INVENTORY_ITEM *item = M_FindItem(object_id);
    const bool is_new = item == nullptr;
    if (is_new) {
        item = Memory_Alloc(sizeof(*item));
        item->object_id = object_id;
        item->scale = 1.0f;
    }

#define M_READ(key, field) item->field = M_ReadInt(L, 1, key, item->field)
    M_READ("frames_total", frames_total);
    M_READ("current_frame", current_frame);
    M_READ("goal_frame", goal_frame);
    M_READ("open_frame", open_frame);
    M_READ("anim_direction", anim_direction);
    M_READ("anim_speed", anim_speed);
    M_READ("anim_count", anim_count);
    M_READ("x_rot_pt_sel", x_rot_pt_sel);
    M_READ("x_rot_pt", x_rot_pt);
    M_READ("x_rot_sel", x_rot_sel);
    M_READ("x_rot_nosel", x_rot_nosel);
    M_READ("x_rot", x_rot);
    M_READ("y_rot_sel", y_rot_sel);
    M_READ("y_rot", y_rot);
    M_READ("y_trans_sel", y_trans_sel);
    M_READ("y_trans", y_trans);
    M_READ("z_trans_sel", z_trans_sel);
    M_READ("z_trans", z_trans);
    item->scale = M_ReadNum(L, 1, "scale", item->scale);
    M_READ("y_offset", y_offset);
    M_READ("base_rot_x", base_rot.x);
    M_READ("base_rot_y", base_rot.y);
    M_READ("base_rot_z", base_rot.z);
    lua_getfield(L, 1, "draws_at_pivot");
    if (!lua_isnil(L, -1)) {
        item->draws_at_pivot = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    M_READ("meshes_sel", meshes_sel);
    M_READ("meshes_drawn", meshes_drawn);
    M_READ("inv_pos", inv_pos);
#undef M_READ

    if (is_new) {
        Vector_Add(g_InvRing_Items, &item);
    }
    return 0;
}

// trxc.inventory_ring.item(object_id) -> table or nil
static int M_L_RingItem(lua_State *const L)
{
    const INVENTORY_ITEM *const item = M_FindItem(LUA_CheckObjectID(L, 1));
    if (item == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, 0, 28);
    lua_pushstring(L, Catalog_IDToKey(CATALOG_OBJECTS, item->object_id));
    lua_setfield(L, -2, "object_id");
#define M_WRITE(key, field)                                                    \
    lua_pushinteger(L, item->field);                                           \
    lua_setfield(L, -2, key)
    M_WRITE("frames_total", frames_total);
    M_WRITE("current_frame", current_frame);
    M_WRITE("goal_frame", goal_frame);
    M_WRITE("open_frame", open_frame);
    M_WRITE("anim_direction", anim_direction);
    M_WRITE("anim_speed", anim_speed);
    M_WRITE("anim_count", anim_count);
    M_WRITE("x_rot_pt_sel", x_rot_pt_sel);
    M_WRITE("x_rot_pt", x_rot_pt);
    M_WRITE("x_rot_sel", x_rot_sel);
    M_WRITE("x_rot_nosel", x_rot_nosel);
    M_WRITE("x_rot", x_rot);
    M_WRITE("y_rot_sel", y_rot_sel);
    M_WRITE("y_rot", y_rot);
    M_WRITE("y_trans_sel", y_trans_sel);
    M_WRITE("y_trans", y_trans);
    M_WRITE("z_trans_sel", z_trans_sel);
    M_WRITE("z_trans", z_trans);
    lua_pushnumber(L, item->scale);
    lua_setfield(L, -2, "scale");
    M_WRITE("y_offset", y_offset);
    M_WRITE("base_rot_x", base_rot.x);
    M_WRITE("base_rot_y", base_rot.y);
    M_WRITE("base_rot_z", base_rot.z);
    lua_pushboolean(L, item->draws_at_pivot);
    lua_setfield(L, -2, "draws_at_pivot");
    M_WRITE("meshes_sel", meshes_sel);
    M_WRITE("meshes_drawn", meshes_drawn);
    M_WRITE("inv_pos", inv_pos);
#undef M_WRITE
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "declare_item", M_L_RingDeclareItem },
    { "icon_of", M_L_RingIconOf },
    { "item", M_L_RingItem },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "inventory_ring", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
