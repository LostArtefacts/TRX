#include <trx/core/vector.h>
#include <trx/game/console/common.h>
#include <trx/game/inject.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

static lua_State *m_L = nullptr;
static VECTOR *m_Declarations = nullptr;

// Read the injection names returned by a declaration.
static void M_ReadList(const int32_t ref)
{
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(m_L, 0, 1, 0) != LUA_OK) {
        Console_ShowError(
            "injection declaration error: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return;
    }
    if (!lua_istable(m_L, -1)) {
        if (!lua_isnil(m_L, -1)) {
            Console_ShowError(
                "an injection declaration must answer with a list");
        }
        lua_pop(m_L, 1);
        return;
    }

    const int32_t count = (int32_t)lua_rawlen(m_L, -1);
    for (int32_t i = 1; i <= count; i++) {
        lua_rawgeti(m_L, -1, i);
        const char *const name = lua_tostring(m_L, -1);
        if (name != nullptr) {
            Inject_AddDeclaredInjection(name);
        } else {
            Console_ShowError("an injection declaration must name files");
        }
        lua_pop(m_L, 1);
    }
    lua_pop(m_L, 1);
}

static void M_Collect(void)
{
    for (int32_t i = 0; m_Declarations != nullptr && i < m_Declarations->count;
         i++) {
        M_ReadList(*(int32_t *)Vector_Get(m_Declarations, i));
    }
}

// trxc.inject.declare(func)
static int M_L_Declare(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    if (m_Declarations == nullptr) {
        m_Declarations = Vector_Create(sizeof(int32_t));
    }
    lua_pushvalue(L, 1);
    const int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    Vector_Add(m_Declarations, (void *)&ref);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "declare", M_L_Declare },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    m_L = L;
    LUA_RegisterModule(L, "inject", m_Module);
    Inject_SetDeclarationCollector(M_Collect);
}

static void M_Shutdown(void)
{
    Inject_SetDeclarationCollector(nullptr);
    if (m_Declarations != nullptr) {
        Vector_Free(m_Declarations);
        m_Declarations = nullptr;
    }
    m_L = nullptr;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
