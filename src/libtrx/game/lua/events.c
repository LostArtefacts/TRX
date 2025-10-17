#include "game/lua/events.h"

#include "game/lua/common.h"
#include "log.h"
#include "vector.h"

#include <lauxlib.h>
#include <lua.h>

typedef struct {
    int32_t ref;
    LUA_EVENT_TYPE type;
    bool level_scoped;
} M_LISTENER;

static lua_State *m_L = nullptr;
static VECTOR *m_Listeners = nullptr;

// trxc.events.attach(event_type, callback) → id
static int32_t M_L_EventsAttach(lua_State *const L)
{
    const LUA_EVENT_TYPE ev = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    const int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (m_Listeners == nullptr) {
        m_Listeners = Vector_Create(sizeof(M_LISTENER));
    }
    const M_LISTENER listener = {
        .ref = ref,
        .type = ev,
        .level_scoped = Lua_GetScriptContext() == LUA_CONTEXT_LEVEL,
    };
    Vector_Add(m_Listeners, &listener);
    lua_pushinteger(L, ref);
    return 1;
}

// trxc.events.detach(id)
static int32_t M_L_EventsDetach(lua_State *const L)
{
    int32_t id = luaL_checkinteger(L, 1);
    if (m_Listeners == nullptr) {
        return 0;
    }
    for (int32_t i = 0; i < m_Listeners->count; i++) {
        const M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->ref == id) {
            luaL_unref(L, LUA_REGISTRYINDEX, lst->ref);
            Vector_RemoveAt(m_Listeners, i);
            break;
        }
    }
    return 0;
}

void Lua_ClearLevelListeners(void)
{
    lua_State *const L = m_L;
    if (L == nullptr) {
        return;
    }
    if (m_Listeners == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Listeners->count;) {
        M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->level_scoped) {
            luaL_unref(L, LUA_REGISTRYINDEX, lst->ref);
            Vector_RemoveAt(m_Listeners, i);
        } else {
            i++;
        }
    }
}

void Lua_FireEvent(LUA_EVENT_TYPE ev, int32_t arg)
{
    lua_State *const L = m_L;
    if (L == nullptr || m_Listeners == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Listeners->count; i++) {
        M_LISTENER *const lst = Vector_Get(m_Listeners, i);
        if (lst->type != ev) {
            continue;
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, lst->ref);
        lua_pushinteger(L, arg);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            LOG_ERROR("Lua event handler error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
}

void LUA_CreateEvents(lua_State *const L)
{
    m_L = L;
    lua_getglobal(L, "trxc");
    lua_newtable(L);

    lua_pushcfunction(L, M_L_EventsAttach);
    lua_setfield(L, -2, "attach");
    lua_pushcfunction(L, M_L_EventsDetach);
    lua_setfield(L, -2, "detach");

    lua_newtable(L);
    lua_pushinteger(L, LUA_EVENT_LEVEL_START);
    lua_setfield(L, -2, "LEVEL_START");
    lua_pushinteger(L, LUA_EVENT_LEVEL_LOAD);
    lua_setfield(L, -2, "LEVEL_LOAD");
    lua_pushinteger(L, LUA_EVENT_PICKUP);
    lua_setfield(L, -2, "PICKUP");
    lua_pushinteger(L, LUA_EVENT_CONTROL_PRE);
    lua_setfield(L, -2, "CONTROL");
    lua_pushinteger(L, LUA_EVENT_CONTROL_POST);
    lua_setfield(L, -2, "CONTROL_POST");
    lua_setfield(L, -2, "EventType");

    lua_setfield(L, -2, "events");
    lua_pop(L, 1);
}
