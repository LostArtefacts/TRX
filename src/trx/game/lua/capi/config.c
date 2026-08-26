#include <trx/config/common.h>
#include <trx/config/option.h>
#include <trx/config/registry.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/game/console/common.h>
#include <trx/game/level/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <string.h>

// How deep a watcher writing a setting may go. Answering a change by changing
// another setting is the point of a watcher; two watchers answering each other
// is a fight, and this is where it is called off rather than run out of stack.
#define M_MAX_DISPATCH_DEPTH 8

// What a declaration says, read out in full before any of it is registered. A
// declaration half of which was refused would leave an option nobody asked for.
typedef struct {
    const char *key;
    TRX_VALUE default_value;
    CONFIG_OPTION_BOUNDS bounds;
    bool has_bounds;
} M_DECLARATION;

// A script waiting to hear that a setting moved. The key is kept rather than
// the option: an option is dropped and made again with the game, and a watcher
// has no business holding an address that dies.
typedef struct {
    int32_t id;
    char *key;
    int32_t fn_ref;
    // Set up by a level script, so it goes when the level does, as a
    // trx.events listener does.
    bool level_scoped;
    // Still owed the call that every watcher gets for the value in force. A
    // level script attaches before the level is read, where a handler can
    // reach nothing the level carries, so the call waits for the world.
    bool pending_initial;
    bool dead;
} M_WATCHER;

static lua_State *m_L = nullptr;
static VECTOR *m_Watchers = nullptr;
static int32_t m_NextWatcherId = 1;
static int32_t m_ChangeListener = -1;
static int32_t m_DispatchDepth = 0;

static CONFIG_OPTION *M_GetOption(lua_State *const L, const int32_t arg)
{
    const char *const key = luaL_checkstring(L, arg);
    CONFIG_OPTION *const option = Config_FindOption(key);
    if (option == nullptr) {
        luaL_error(L, "unknown option: %s", key);
    }
    return option;
}

// A value is handed to the config string parser, so it is spelled the way that
// parser reads it. A boolean has to be spelled out: Lua's own conversion would
// make it "1".
static const char *M_ValueAsString(lua_State *const L, const int32_t arg)
{
    if (lua_isboolean(L, arg)) {
        return lua_toboolean(L, arg) ? "true" : "false";
    }
    if (lua_isnumber(L, arg)) {
        return lua_tostring(L, arg);
    }
    // A color arrives as the value trx.math declares, which spells itself as
    // the hex text the parser reads.
    if (lua_istable(L, arg)
        && luaL_getmetafield(L, arg, "__tostring") != LUA_TNIL) {
        lua_pop(L, 1);
        return luaL_tolstring(L, arg, nullptr);
    }
    return luaL_checkstring(L, arg);
}

// trxc.config.get(key)
static int M_L_ConfigGet(lua_State *const L)
{
    LUA_Config_PushOptionValue(L, M_GetOption(L, 1));
    return 1;
}

// The name a value shape goes by in a description.
static const char *M_KindName(const TRX_VALUE_TYPE type)
{
    switch (type) {
    case TVT_BOOL:
        return "boolean";
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        return "integer";
    case TVT_FLOAT:
    case TVT_DOUBLE:
        return "number";
    case TVT_XYZ_16:
    case TVT_XYZ_32:
        return "xyz";
    case TVT_RGB_888:
    case TVT_RGB_F:
        return "color";
    case TVT_ENUM:
        return "enum";
    case TVT_DYNAMIC_ENUM:
        return "dynamic_enum";
    case TVT_STRING:
        return "string";
    }
    return "";
}

// The default reads back the way the value does, so what describes a setting
// is what declares one.
static void M_PushDefault(lua_State *const L, const CONFIG_OPTION *const option)
{
    const TRX_VALUE *const value = &option->default_value;
    if (value->type == TVT_ENUM || value->type == TVT_STRING
        || value->type == TVT_DYNAMIC_ENUM) {
        lua_pushstring(
            L,
            Value_Format(
                value->type, Config_Option_GetEnumKey(option), value, false));
        return;
    }
    LUA_PushValue(L, value);
}

// trxc.config.describe(key) -> table
static int M_L_ConfigDescribe(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    lua_newtable(L);
    lua_pushstring(L, option->name);
    lua_setfield(L, -2, "key");
    lua_pushstring(L, M_KindName(option->value.type));
    lua_setfield(L, -2, "kind");
    lua_pushboolean(L, (option->flags & CONFIG_OPTION_PERCENT) != 0);
    lua_setfield(L, -2, "percent");
    M_PushDefault(L, option);
    lua_setfield(L, -2, "default");
    if (option->bounds != nullptr) {
        lua_pushnumber(L, option->bounds->min);
        lua_setfield(L, -2, "min");
        lua_pushnumber(L, option->bounds->max);
        lua_setfield(L, -2, "max");
    }

    if (option->value.type == TVT_ENUM) {
        VECTOR *const values = EnumMap_ListValues(option->enum_map);
        lua_createtable(L, values != nullptr ? values->count : 0, 0);
        if (values != nullptr) {
            for (int32_t i = 0; i < values->count; i++) {
                lua_pushstring(L, *(char **)Vector_Get(values, i));
                lua_rawseti(L, -2, i + 1);
            }
            Vector_Free(values);
        }
        lua_setfield(L, -2, "values");
    } else if (option->value.type == TVT_DYNAMIC_ENUM) {
        const void *const token = Config_Option_GetEnumKey(option);
        const int32_t count = DynamicEnum_GetValueCount(token);
        lua_createtable(L, count, 0);
        int32_t n = 0;
        for (int32_t i = 0; i < count; i++) {
            const char *const value = DynamicEnum_GetValueAt(token, i);
            if (value != nullptr) {
                lua_pushstring(L, value);
                n++;
                lua_rawseti(L, -2, n);
            }
        }
        lua_setfield(L, -2, "values");
    }
    return 1;
}

// trxc.config.set(key, value, force?)
static int M_L_ConfigSet(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    const bool force = lua_toboolean(L, 3);
    if (!Config_Option_SetFromString(option, new_value, force)) {
        return luaL_error(
            L, "failed to set option %s to %s", option->name, new_value);
    }
    Config_Update();
    return 0;
}

// trxc.config.reset(key, force?) -> bool
static int M_L_ConfigReset(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const bool force = lua_toboolean(L, 2);
    const bool changed = Config_Option_RestoreDefault(option, force);
    if (changed) {
        Config_Update();
    }
    lua_pushboolean(L, changed);
    return 1;
}

// trxc.config.override(key, value)
static int M_L_ConfigOverride(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const char *const new_value = M_ValueAsString(L, 2);
    if (!Config_Option_PushHoldFromString(
            option, new_value, CONFIG_HOLD_SCRIPT)) {
        return luaL_error(
            L, "failed to override option %s with %s", option->name, new_value);
    }
    Config_Update();
    return 0;
}

// trxc.config.restore(key) -> bool
static int M_L_ConfigRestore(lua_State *const L)
{
    CONFIG_OPTION *const option = M_GetOption(L, 1);
    const bool restored = Config_Option_PopHold(option);
    if (restored) {
        Config_Update();
    }
    lua_pushboolean(L, restored);
    return 1;
}

// trxc.config.is_overridden(key) -> bool
static int M_L_ConfigIsOverridden(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    lua_pushboolean(L, Config_Option_IsHeld(option));
    return 1;
}

// trxc.config.list()
static int M_L_ConfigList(lua_State *const L)
{
    lua_newtable(L);
    for (CONFIG_OPTION *const *option = Config_GetOptions(); *option != nullptr;
         option++) {
        LUA_Config_PushOptionValue(L, *option);
        lua_setfield(L, -2, (*option)->name);
    }
    return 1;
}

// A string field of a declaration. A number is refused rather than coerced:
// luaL_checkstring would rewrite the table's own field into a string and hand
// back a pointer into the stack slot this pops.
static const char *M_StrField(
    lua_State *const L, const int32_t idx, const char *const name,
    const bool required)
{
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) && !required) {
        lua_pop(L, 1);
        return nullptr;
    }
    if (lua_type(L, -1) != LUA_TSTRING) {
        luaL_error(L, "option field '%s' must be a string", name);
    }
    // Handed back rather than copied: the string belongs to the spec table,
    // which is an argument of the call being read.
    const char *const result = lua_tostring(L, -1);
    lua_pop(L, 1);
    return result;
}

// The kinds a script may declare, named the way trxc.config.describe reports
// them back. The rest of the taxonomy names storage the engine owns.
static TRX_VALUE_TYPE M_ReadKind(lua_State *const L, const char *const kind)
{
    if (strcmp(kind, "boolean") == 0) {
        return TVT_BOOL;
    }
    if (strcmp(kind, "integer") == 0) {
        return TVT_S32;
    }
    if (strcmp(kind, "dynamic_enum") == 0) {
        return TVT_DYNAMIC_ENUM;
    }
    luaL_error(L, "unknown option kind: %s", kind);
    return TVT_BOOL;
}

// Whether the values the declaration lists name the one it defaults to. An
// option defaulting to a value it does not offer has nothing to fall back on.
static bool M_ListsDefault(
    lua_State *const L, const int32_t idx, const char *const default_value)
{
    bool found = false;
    const int32_t count = (int32_t)lua_rawlen(L, idx);
    for (int32_t i = 1; i <= count && !found; i++) {
        lua_rawgeti(L, idx, i);
        if (lua_type(L, -1) != LUA_TSTRING) {
            luaL_error(L, "option values must be strings");
        }
        found = strcmp(lua_tostring(L, -1), default_value) == 0;
        lua_pop(L, 1);
    }
    return found;
}

static M_DECLARATION M_ReadDeclaration(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    M_DECLARATION spec = {};
    spec.key = M_StrField(L, 1, "key", true);
    const TRX_VALUE_TYPE type = M_ReadKind(L, M_StrField(L, 1, "kind", true));

    lua_getfield(L, 1, "default");
    switch (type) {
    case TVT_BOOL:
        luaL_checktype(L, -1, LUA_TBOOLEAN);
        spec.default_value =
            (TRX_VALUE) { .type = TVT_BOOL, .as_bool = lua_toboolean(L, -1) };
        break;
    case TVT_S32:
        spec.default_value =
            (TRX_VALUE) { .type = TVT_S32, .as_int = luaL_checkinteger(L, -1) };
        break;
    default:
        spec.default_value = (TRX_VALUE) { .type = TVT_DYNAMIC_ENUM };
        break;
    }
    lua_pop(L, 1);
    if (type == TVT_DYNAMIC_ENUM) {
        spec.default_value.as_str = M_StrField(L, 1, "default", true);
    }

    // Each bound stands on its own: naming a floor is not asking for a ceiling
    // of zero, so what the declaration leaves out is as far as the storage
    // goes.
    lua_getfield(L, 1, "min");
    lua_getfield(L, 1, "max");
    spec.has_bounds = !lua_isnil(L, -2) || !lua_isnil(L, -1);
    spec.bounds.min = (double)luaL_optinteger(L, -2, INT32_MIN);
    spec.bounds.max = (double)luaL_optinteger(L, -1, INT32_MAX);
    lua_pop(L, 2);
    if (type != TVT_S32) {
        spec.has_bounds = false;
    }
    if (spec.has_bounds && spec.bounds.min > spec.bounds.max) {
        luaL_error(
            L, "option %s takes nothing: min %d is above max %d", spec.key,
            (int32_t)spec.bounds.min, (int32_t)spec.bounds.max);
    }
    if (spec.has_bounds
        && (spec.default_value.as_int < (int64_t)spec.bounds.min
            || spec.default_value.as_int > (int64_t)spec.bounds.max)) {
        luaL_error(
            L, "option %s defaults to %d, outside %d..%d", spec.key,
            (int32_t)spec.default_value.as_int, (int32_t)spec.bounds.min,
            (int32_t)spec.bounds.max);
    }

    if (type == TVT_DYNAMIC_ENUM) {
        lua_getfield(L, 1, "values");
        if (!lua_istable(L, -1) || lua_rawlen(L, -1) == 0) {
            luaL_error(
                L, "option %s is an enum but declares no values", spec.key);
        }
        if (!M_ListsDefault(L, lua_gettop(L), spec.default_value.as_str)) {
            luaL_error(
                L, "option %s defaults to %s, which it does not list", spec.key,
                spec.default_value.as_str);
        }
        lua_pop(L, 1);
    }
    return spec;
}

// Registers one value the option takes. Each label is a game string key derived
// from the option, so a declared value is translated as every other one is.
static void M_AddValue(
    const CONFIG_OPTION *const option, const char *const value)
{
    char *label = String_Format("settings/%s/values/%s", option->name, value);
    DynamicEnum_AddValue(Config_Option_GetEnumKey(option), value, label);
    Memory_FreePointer(&label);
}

// Seeds a declared enum's values.
static void M_SeedValues(lua_State *const L, const CONFIG_OPTION *const option)
{
    DynamicEnum_ResetValues(Config_Option_GetEnumKey(option));
    lua_getfield(L, 1, "values");
    const int32_t count = (int32_t)lua_rawlen(L, -1);
    for (int32_t i = 1; i <= count; i++) {
        lua_rawgeti(L, -1, i);
        M_AddValue(option, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

// A saved value this declaration does not list belongs to another game sharing
// the settings file. Kept and turned off rather than dropped, so that game gets
// it back.
static void M_KeepUnlistedValue(
    const CONFIG_OPTION *const option, const char *const value)
{
    if (value == nullptr
        || DynamicEnum_IsValidValue(Config_Option_GetEnumKey(option), value)) {
        return;
    }
    M_AddValue(option, value);
    DynamicEnum_SetValueEnabled(Config_Option_GetEnumKey(option), value, false);
}

// trxc.config.declare(spec)
static int M_L_ConfigDeclare(lua_State *const L)
{
    const M_DECLARATION spec = M_ReadDeclaration(L);
    CONFIG_OPTION *const option = Config_Register(&(CONFIG_OPTION_DESC) {
        .name = spec.key,
        .default_value = spec.default_value,
        .bounds = spec.has_bounds ? &spec.bounds : nullptr,
    });
    if (option == nullptr) {
        return luaL_error(L, "option already declared: %s", spec.key);
    }
    if (option->value.type == TVT_DYNAMIC_ENUM) {
        M_SeedValues(L, option);
        M_KeepUnlistedValue(option, option->value.as_str);
        M_KeepUnlistedValue(option, Config_Option_GetBaseValue(option)->as_str);
    }
    return 0;
}

// Tells one watcher what a setting holds. A watcher that raises must not take
// the rest of them with it: the setting has moved either way, and the others
// still have to hear about it.
static void M_CallWatcher(
    lua_State *const L, M_WATCHER *const watcher,
    const CONFIG_OPTION *const option)
{
    watcher->pending_initial = false;
    lua_rawgeti(L, LUA_REGISTRYINDEX, watcher->fn_ref);
    LUA_Config_PushOptionValue(L, option);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        Console_ShowError(
            "config watcher for %s failed: %s", option->name,
            lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void M_FreeWatcher(lua_State *const L, M_WATCHER *const watcher)
{
    luaL_unref(L, LUA_REGISTRYINDEX, watcher->fn_ref);
    Memory_FreePointer(&watcher->key);
}

// Drops the watchers marked dead. Removing one mid-dispatch would move the
// ones after it out from under the walk, so a watcher goes as soon as the last
// dispatch is over and not before.
static void M_CompactWatchers(lua_State *const L)
{
    for (int32_t i = m_Watchers->count - 1; i >= 0; i--) {
        M_WATCHER *const watcher = Vector_Get(m_Watchers, i);
        if (watcher->dead) {
            M_FreeWatcher(L, watcher);
            Vector_RemoveAt(m_Watchers, i);
        }
    }
}

// Says a setting moved, to whoever asked about that one.
static void M_HandleChange(const EVENT *const event, void *const user_data)
{
    lua_State *const L = m_L;
    const CONFIG_CHANGE *const change = event->data;
    if (L == nullptr || m_Watchers == nullptr || change == nullptr) {
        return;
    }
    if (m_DispatchDepth >= M_MAX_DISPATCH_DEPTH) {
        Console_ShowError(
            "config watchers are still changing settings %d calls deep; "
            "giving up on this change",
            m_DispatchDepth);
        return;
    }

    m_DispatchDepth++;
    // The watchers as they stand: one attached from inside another has already
    // been told what the setting holds, and has nothing to hear from this
    // round.
    const int32_t count = m_Watchers->count;
    for (int32_t i = 0; i < change->count; i++) {
        const CONFIG_OPTION *const option = change->options[i];
        for (int32_t j = 0; j < count && j < m_Watchers->count; j++) {
            M_WATCHER *const watcher = Vector_Get(m_Watchers, j);
            if (watcher->dead || strcmp(watcher->key, option->name) != 0) {
                continue;
            }
            M_CallWatcher(L, watcher, option);
        }
    }
    m_DispatchDepth--;
    if (m_DispatchDepth == 0) {
        M_CompactWatchers(L);
    }
}

// trxc.config.on_change(key, fn) -> id
static int M_L_ConfigOnChange(lua_State *const L)
{
    const CONFIG_OPTION *const option = M_GetOption(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (m_Watchers == nullptr) {
        m_Watchers = Vector_Create(sizeof(M_WATCHER));
    }
    lua_pushvalue(L, 2);
    // Taken before the call below, which may attach a watcher of its own and
    // leave the counter naming that one instead.
    const int32_t id = m_NextWatcherId++;
    const bool level_scoped = LUA_GetScriptContext() == LUA_CONTEXT_LEVEL;
    const bool wait_for_world = level_scoped && !Level_IsWorldLoaded();
    Vector_Add(
        m_Watchers,
        &(M_WATCHER) {
            .id = id,
            .key = Memory_DupStr(option->name),
            .fn_ref = luaL_ref(L, LUA_REGISTRYINDEX),
            .level_scoped = level_scoped,
            .pending_initial = wait_for_world,
        });
    // The setting already holds something, and a script asking to hear about it
    // is asking about the value in force as much as the ones to come.
    if (!wait_for_world) {
        M_CallWatcher(L, Vector_Get(m_Watchers, m_Watchers->count - 1), option);
    }
    lua_pushinteger(L, id);
    return 1;
}

// trxc.config.off_change(id) -> bool
static int M_L_ConfigOffChange(lua_State *const L)
{
    const int32_t id = (int32_t)luaL_checkinteger(L, 1);
    for (int32_t i = 0; m_Watchers != nullptr && i < m_Watchers->count; i++) {
        M_WATCHER *const watcher = Vector_Get(m_Watchers, i);
        if (watcher->id != id || watcher->dead) {
            continue;
        }
        watcher->dead = true;
        if (m_DispatchDepth == 0) {
            M_CompactWatchers(L);
        }
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "get", M_L_ConfigGet },
    { "describe", M_L_ConfigDescribe },
    { "set", M_L_ConfigSet },
    { "reset", M_L_ConfigReset },
    { "override", M_L_ConfigOverride },
    { "restore", M_L_ConfigRestore },
    { "is_overridden", M_L_ConfigIsOverridden },
    { "list", M_L_ConfigList },
    { "declare", M_L_ConfigDeclare },
    { "on_change", M_L_ConfigOnChange },
    { "off_change", M_L_ConfigOffChange },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    m_L = L;
    m_ChangeListener = Config_SubscribeChanges(M_HandleChange, nullptr);
    LUA_RegisterModule(L, "config", m_Module);
}

// A ref names a slot in the registry of the state that is going, and the next
// state numbers its own from scratch. Kept, it would name the slot's next
// occupant.
static void M_Shutdown(void)
{
    for (int32_t i = 0; m_Watchers != nullptr && i < m_Watchers->count; i++) {
        M_WATCHER *const watcher = Vector_Get(m_Watchers, i);
        Memory_FreePointer(&watcher->key);
    }
    if (m_Watchers != nullptr) {
        Vector_Free(m_Watchers);
        m_Watchers = nullptr;
    }
    if (m_ChangeListener >= 0) {
        Config_UnsubscribeChanges(m_ChangeListener);
        m_ChangeListener = -1;
    }
    m_DispatchDepth = 0;
    m_L = nullptr;
}

// The option's declared type is what a script gets back: a bool reads as a
// bool, a number as a number, and a color as a trx.math.Color.
void LUA_Config_PushOptionValue(
    lua_State *const L, const CONFIG_OPTION *const option)
{
    // Enums, strings and dynamic enums read back as their name or display
    // string, which is also how a script gives them on the way in.
    const TRX_VALUE_TYPE type = option->value.type;
    if (type == TVT_ENUM || type == TVT_STRING || type == TVT_DYNAMIC_ENUM) {
        lua_pushstring(L, Config_Option_GetValueAsString(option, false));
        return;
    }
    LUA_PushValue(L, &option->value);
}

void LUA_Config_FlushPendingWatchers(void)
{
    lua_State *const L = m_L;
    if (L == nullptr || m_Watchers == nullptr) {
        return;
    }
    m_DispatchDepth++;
    for (int32_t i = 0; i < m_Watchers->count; i++) {
        M_WATCHER *const watcher = Vector_Get(m_Watchers, i);
        if (watcher->dead || !watcher->pending_initial) {
            continue;
        }
        const CONFIG_OPTION *const option = Config_FindOption(watcher->key);
        if (option == nullptr) {
            watcher->pending_initial = false;
            continue;
        }
        M_CallWatcher(L, watcher, option);
    }
    m_DispatchDepth--;
    if (m_DispatchDepth == 0) {
        M_CompactWatchers(L);
    }
}

void LUA_Config_ClearLevelWatchers(void)
{
    lua_State *const L = m_L;
    if (L == nullptr || m_Watchers == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_Watchers->count; i++) {
        M_WATCHER *const watcher = Vector_Get(m_Watchers, i);
        watcher->dead = watcher->dead || watcher->level_scoped;
    }
    if (m_DispatchDepth == 0) {
        M_CompactWatchers(L);
    }
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
