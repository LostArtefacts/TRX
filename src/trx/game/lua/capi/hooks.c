#include <trx/core/memory.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/console.h>
#include <trx/game/items.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/setup.h>
#include <trx/game/objects/types.h>

#include <string.h>

// Lets a script stand in for one of an object's engine functions. An object
// has at most one script per function, which keeps what the object had there
// so that the script can defer to it. Hooks outlive a level: object records are
// rebuilt for each level, and the hooks are put back once they are.

typedef void (*M_FUNC)(void);

typedef enum {
    M_SLOT_CONTROL,
    M_SLOT_INITIALISE,
    M_SLOT_NUMBER_OF,
} M_SLOT_ID;

typedef struct {
    const char *name;
    M_FUNC (*get)(const OBJECT *obj);
    void (*set)(OBJECT *obj, M_FUNC func);
    M_FUNC trampoline;
} M_SLOT;

typedef struct {
    int32_t ref;
    M_FUNC original;
} M_HOOK;

static M_HOOK *m_Hooks = nullptr;
static int32_t m_ObjectCount = 0;
static lua_State *m_L = nullptr;

static M_HOOK *M_GetHook(const OBJECT_ID object_id, const M_SLOT_ID slot)
{
    if (object_id < 0 || object_id >= m_ObjectCount) {
        return nullptr;
    }
    M_HOOK *const hook = &m_Hooks[object_id * M_SLOT_NUMBER_OF + slot];
    return hook->ref == LUA_NOREF ? nullptr : hook;
}

// Calls the script for an item, leaving `result_count` results on the stack.
// Errors are reported to the console, and leave nothing.
static bool M_CallForItem(
    const M_HOOK *const hook, const char *const name, const int16_t item_num,
    const int result_count)
{
    if (m_L == nullptr) {
        return false;
    }
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook->ref);
    LUA_PushItem(m_L, item_num);
    if (lua_pcall(m_L, 1, result_count, 0) != LUA_OK) {
        Console_ShowError("%s hook error: %s", name, lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
        return false;
    }
    return true;
}

// Control and initialise give the script nothing to hand back, so a script
// that stands in for them replaces them outright.
static void M_Control(const int16_t item_num)
{
    const M_HOOK *const hook =
        M_GetHook(Item_Get(item_num)->object_id, M_SLOT_CONTROL);
    if (hook != nullptr) {
        M_CallForItem(hook, "control", item_num, 0);
    }
}

static void M_Initialise(const int16_t item_num)
{
    const M_HOOK *const hook =
        M_GetHook(Item_Get(item_num)->object_id, M_SLOT_INITIALISE);
    if (hook != nullptr) {
        M_CallForItem(hook, "initialise", item_num, 0);
    }
}

static M_FUNC M_GetControlFunc(const OBJECT *const obj)
{
    return (M_FUNC)obj->control_func;
}

static void M_SetControlFunc(OBJECT *const obj, const M_FUNC func)
{
    obj->control_func = (void (*)(int16_t))func;
}

static M_FUNC M_GetInitialiseFunc(const OBJECT *const obj)
{
    return (M_FUNC)obj->initialise_func;
}

static void M_SetInitialiseFunc(OBJECT *const obj, const M_FUNC func)
{
    obj->initialise_func = (void (*)(int16_t))func;
}

static const M_SLOT m_Slots[M_SLOT_NUMBER_OF] = {
    [M_SLOT_CONTROL] = {
        .name = "control",
        .get = M_GetControlFunc,
        .set = M_SetControlFunc,
        .trampoline = (M_FUNC)M_Control,
    },
    [M_SLOT_INITIALISE] = {
        .name = "initialise",
        .get = M_GetInitialiseFunc,
        .set = M_SetInitialiseFunc,
        .trampoline = (M_FUNC)M_Initialise,
    },
};

static void M_Apply(const OBJECT_ID object_id, const M_SLOT_ID slot)
{
    M_HOOK *const hook = M_GetHook(object_id, slot);
    OBJECT *const obj = Object_TryGet(object_id);
    if (hook == nullptr || obj == nullptr) {
        return;
    }
    const M_FUNC current = m_Slots[slot].get(obj);
    if (current != m_Slots[slot].trampoline) {
        hook->original = current;
        m_Slots[slot].set(obj, m_Slots[slot].trampoline);
    }
}

static void M_Restore(const OBJECT_ID object_id, const M_SLOT_ID slot)
{
    const M_HOOK *const hook = M_GetHook(object_id, slot);
    OBJECT *const obj = Object_TryGet(object_id);
    if (hook != nullptr && obj != nullptr
        && m_Slots[slot].get(obj) == m_Slots[slot].trampoline) {
        m_Slots[slot].set(obj, hook->original);
    }
}

static void M_ApplyAll(void)
{
    for (int32_t i = 0; i < m_ObjectCount * M_SLOT_NUMBER_OF; i++) {
        if (m_Hooks[i].ref != LUA_NOREF) {
            M_Apply(i / M_SLOT_NUMBER_OF, i % M_SLOT_NUMBER_OF);
        }
    }
}

// Grows the table to cover every object, including ones minted since.
static void M_Reserve(const int32_t object_count)
{
    if (object_count <= m_ObjectCount) {
        return;
    }
    m_Hooks = Memory_Realloc(
        m_Hooks, sizeof(M_HOOK) * object_count * M_SLOT_NUMBER_OF);
    for (int32_t i = m_ObjectCount * M_SLOT_NUMBER_OF;
         i < object_count * M_SLOT_NUMBER_OF; i++) {
        m_Hooks[i] = (M_HOOK) { .ref = LUA_NOREF };
    }
    m_ObjectCount = object_count;
}

static M_SLOT_ID M_CheckSlot(lua_State *const L, const int arg)
{
    const char *const name = luaL_checkstring(L, arg);
    for (M_SLOT_ID slot = 0; slot < M_SLOT_NUMBER_OF; slot++) {
        if (strcmp(m_Slots[slot].name, name) == 0) {
            return slot;
        }
    }
    luaL_argerror(L, arg, lua_pushfstring(L, "no such hook '%s'", name));
    return M_SLOT_NUMBER_OF;
}

// trxc.hooks.set_object(object: trx.objects.Object, slot: string, fn?: function)
static int M_L_SetObject(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, Type_GetByName("OBJECT"));
    const OBJECT_ID object_id = (OBJECT_ID)ref->handle.id;
    const M_SLOT_ID slot = M_CheckSlot(L, 2);
    const bool clear = lua_isnoneornil(L, 3);
    if (!clear) {
        luaL_checktype(L, 3, LUA_TFUNCTION);
    }

    M_HOOK *const old = M_GetHook(object_id, slot);
    if (old != nullptr) {
        M_Restore(object_id, slot);
        luaL_unref(L, LUA_REGISTRYINDEX, old->ref);
        *old = (M_HOOK) { .ref = LUA_NOREF };
    }
    if (clear) {
        return 0;
    }

    M_Reserve(Catalog_GetCount(CATALOG_OBJECTS));
    lua_pushvalue(L, 3);
    m_Hooks[object_id * M_SLOT_NUMBER_OF + slot] = (M_HOOK) {
        .ref = luaL_ref(L, LUA_REGISTRYINDEX),
    };
    M_Apply(object_id, slot);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "set_object", M_L_SetObject },
    { nullptr, nullptr },
};

static void M_Shutdown(void)
{
    for (int32_t i = 0; i < m_ObjectCount * M_SLOT_NUMBER_OF; i++) {
        const M_HOOK *const hook = &m_Hooks[i];
        if (hook->ref == LUA_NOREF) {
            continue;
        }
        M_Restore(i / M_SLOT_NUMBER_OF, i % M_SLOT_NUMBER_OF);
        if (m_L != nullptr) {
            luaL_unref(m_L, LUA_REGISTRYINDEX, hook->ref);
        }
    }
    Memory_Free(m_Hooks);
    m_Hooks = nullptr;
    m_ObjectCount = 0;
    m_L = nullptr;
}

static void M_Create(lua_State *const L)
{
    m_L = L;
    Object_AddSetupHook(M_ApplyAll);
    LUA_RegisterModule(L, "hooks", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
