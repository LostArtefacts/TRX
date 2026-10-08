#include <trx/core/memory.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/console.h>
#include <trx/game/items.h>
#include <trx/game/lua/hooks/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/setup.h>
#include <trx/game/objects/types.h>

#include <stddef.h>
#include <string.h>

// The object functions a script can stand in for. Each hook is named by the
// object it is for, and swaps its trampoline into the object's record, keeping
// what was there so that the script can defer to it.

typedef void (*M_FUNC)(void);

typedef struct {
    size_t offset;
    M_FUNC trampoline;
    M_FUNC *originals;
    int32_t object_count;
} M_SLOT;

static void M_Control(int16_t item_num);
static void M_Initialise(int16_t item_num);
static ITEM_HIT_EFFECT M_HitEffect(const ITEM *item);

static M_SLOT m_Slots[LUA_HOOK_NUMBER_OF] = {
    [LUA_HOOK_CONTROL] = {
        .offset = offsetof(OBJECT, control_func),
        .trampoline = (M_FUNC)M_Control,
    },
    [LUA_HOOK_INITIALISE] = {
        .offset = offsetof(OBJECT, initialise_func),
        .trampoline = (M_FUNC)M_Initialise,
    },
    [LUA_HOOK_HIT_EFFECT] = {
        .offset = offsetof(OBJECT, get_hit_effect_func),
        .trampoline = (M_FUNC)M_HitEffect,
    },
};

static int32_t M_ReadObject(lua_State *const L, const int arg)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, arg, Type_GetByName("OBJECT"));
    return (int32_t)ref->handle.id;
}

static M_FUNC M_GetFunc(const M_SLOT *const slot, const OBJECT *const obj)
{
    M_FUNC func;
    memcpy(&func, (const char *)obj + slot->offset, sizeof(func));
    return func;
}

static void M_SetFunc(
    const M_SLOT *const slot, OBJECT *const obj, const M_FUNC func)
{
    memcpy((char *)obj + slot->offset, &func, sizeof(func));
}

static void M_Apply(const LUA_HOOK_TYPE hook, const int32_t object_id)
{
    M_SLOT *const slot = &m_Slots[hook];
    OBJECT *const obj = Object_TryGet(object_id);
    if (obj == nullptr || !LUA_Hooks_IsSet(hook, object_id)) {
        return;
    }
    const int32_t object_count = Catalog_GetCount(CATALOG_OBJECTS);
    if (object_count > slot->object_count) {
        slot->originals =
            Memory_Realloc(slot->originals, sizeof(M_FUNC) * object_count);
        memset(
            slot->originals + slot->object_count, 0,
            sizeof(M_FUNC) * (object_count - slot->object_count));
        slot->object_count = object_count;
    }
    const M_FUNC current = M_GetFunc(slot, obj);
    if (current != slot->trampoline) {
        slot->originals[object_id] = current;
        M_SetFunc(slot, obj, slot->trampoline);
    }
}

static void M_Restore(const LUA_HOOK_TYPE hook, const int32_t object_id)
{
    const M_SLOT *const slot = &m_Slots[hook];
    OBJECT *const obj = Object_TryGet(object_id);
    if (obj != nullptr && object_id < slot->object_count
        && M_GetFunc(slot, obj) == slot->trampoline) {
        M_SetFunc(slot, obj, slot->originals[object_id]);
    }
}

// Object records are rebuilt for each level, so the hooks are put back once
// they are.
static void M_ApplyAll(void)
{
    const int32_t object_count = Catalog_GetCount(CATALOG_OBJECTS);
    for (LUA_HOOK_TYPE hook = 0; hook < LUA_HOOK_NUMBER_OF; hook++) {
        if (m_Slots[hook].trampoline == nullptr) {
            continue;
        }
        for (int32_t object_id = 0; object_id < object_count; object_id++) {
            M_Apply(hook, object_id);
        }
    }
}

// Control and initialise give the script nothing to hand back, so a script
// that stands in for them replaces them outright.
static void M_Control(const int16_t item_num)
{
    const int32_t object_id = Item_Get(item_num)->object_id;
    LUA_Hooks_Call(LUA_HOOK_CONTROL, object_id, 0, LUA_ARG_ITEM(item_num));
}

static void M_Initialise(const int16_t item_num)
{
    const int32_t object_id = Item_Get(item_num)->object_id;
    LUA_Hooks_Call(LUA_HOOK_INITIALISE, object_id, 0, LUA_ARG_ITEM(item_num));
}

static ITEM_HIT_EFFECT M_HitEffect(const ITEM *const item)
{
    int32_t effect = LUA_Hooks_CallInt(
        LUA_HOOK_HIT_EFFECT, item->object_id, ITEM_HIT_DEFAULT,
        LUA_ARG_ITEM(Item_GetIndex(item)));
    if (effect < 0 || effect >= ITEM_HIT_NUMBER_OF) {
        Console_ShowError(
            "hit_effect hook error: no such hit effect %d", effect);
        effect = ITEM_HIT_DEFAULT;
    }
    if (effect != ITEM_HIT_DEFAULT) {
        return (ITEM_HIT_EFFECT)effect;
    }

    const M_SLOT *const slot = &m_Slots[LUA_HOOK_HIT_EFFECT];
    const M_FUNC original = item->object_id < slot->object_count
        ? slot->originals[item->object_id]
        : nullptr;
    if (original != nullptr) {
        return ((ITEM_HIT_EFFECT (*)(const ITEM *))original)(item);
    }
    return ITEM_HIT_BLOOD;
}

static void M_Shutdown(void)
{
    for (LUA_HOOK_TYPE hook = 0; hook < LUA_HOOK_NUMBER_OF; hook++) {
        M_SLOT *const slot = &m_Slots[hook];
        for (int32_t object_id = 0; object_id < slot->object_count;
             object_id++) {
            M_Restore(hook, object_id);
        }
        Memory_FreePointer(&slot->originals);
        slot->object_count = 0;
    }
}

static void M_Create(lua_State *const L)
{
    const LUA_HOOK_HANDLERS handlers = {
        .read_key = M_ReadObject,
        .on_set = M_Apply,
        .on_clear = M_Restore,
    };
    LUA_Hooks_SetHandlers(LUA_HOOK_CONTROL, &handlers);
    LUA_Hooks_SetHandlers(LUA_HOOK_INITIALISE, &handlers);
    LUA_Hooks_SetHandlers(LUA_HOOK_HIT_EFFECT, &handlers);
    Object_AddSetupHook(M_ApplyAll);
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
