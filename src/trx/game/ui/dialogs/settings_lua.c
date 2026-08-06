// A settings row a script asked for: where it sits, and what it does that data
// cannot say.
//
// The rest of what a script declares is the config layer's, and the binding for
// it lives with the other Lua modules. A row is not: it belongs to the dialogs,
// and the dialogs sit above the game, so the calls that place one and answer
// for it live here rather than reaching up from there.
//
// One handler is registered per declared row, holding the script's own
// functions by reference. Every callback the dialogs make comes back through
// here, into the function the declaration named.

#include <trx/game/ui/dialogs/settings_lua.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/ui/dialogs/settings_handlers.h>
#include <trx/game/ui/dialogs/settings_rows.h>

#include <lauxlib.h>

typedef enum {
    M_CB_FORMAT_VALUE,
    M_CB_IS_AVAILABLE,
    M_CB_IS_VISIBLE,
    M_CB_CAN_CHANGE_VALUE,
    M_CB_REQUEST_CHANGE_VALUE,
    M_CB_COUNT,
} M_CALLBACK;

// A row a script declared, for as long as the game that declared it. The
// handler is what the dialogs hold, so this never moves: the vector holds
// pointers to these rather than the structs themselves.
typedef struct {
    UI_SETTING_HANDLER handler;
    char *key;
    int32_t refs[M_CB_COUNT];
    // What format_value last handed back. The dialogs read the string after the
    // call returns, and Lua's own copy is gone by then.
    char *formatted;
} M_DECLARED_ROW;

// The names a declaration gives them, in the order above.
static const char *const m_CallbackNames[M_CB_COUNT] = {
    "format_value",     "is_available",         "is_visible",
    "can_change_value", "request_change_value",
};

static lua_State *m_L = nullptr;
static VECTOR *m_Rows = nullptr;

// Pushes the callback and the option's value, ready for the arguments a
// particular one takes. False where the declaration named no such function, and
// nothing is left on the stack.
static bool M_PushCall(
    const M_DECLARED_ROW *const row, const M_CALLBACK cb,
    const CONFIG_OPTION *const option, const bool with_value)
{
    if (m_L == nullptr || row->refs[cb] == LUA_NOREF) {
        return false;
    }
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, row->refs[cb]);
    if (with_value) {
        LUA_Config_PushOptionValue(m_L, option);
    }
    return true;
}

// Makes the call and hands back what it returned, or nothing where it raised.
// A script's own mistake is logged and answered as if it had said nothing: a
// settings row that stops drawing is a worse answer than one that ignores a
// handler.
static bool M_Call(
    const M_DECLARED_ROW *const row, const M_CALLBACK cb, const int32_t nargs)
{
    if (lua_pcall(m_L, nargs, 1, 0) == LUA_OK) {
        return true;
    }
    LOG_ERROR(
        "settings row %s: %s handler failed: %s", row->key, m_CallbackNames[cb],
        lua_tostring(m_L, -1));
    lua_pop(m_L, 1);
    return false;
}

static bool M_CallPredicate(
    const M_DECLARED_ROW *const row, const M_CALLBACK cb,
    const CONFIG_OPTION *const option, const int32_t dir, const bool with_dir,
    const bool absent)
{
    if (!M_PushCall(row, cb, option, true)) {
        return absent;
    }
    if (with_dir) {
        lua_pushinteger(m_L, dir);
    }
    if (!M_Call(row, cb, with_dir ? 2 : 1)) {
        return absent;
    }
    const bool result = lua_toboolean(m_L, -1);
    lua_pop(m_L, 1);
    return result;
}

static const char *M_FormatValue(
    const CONFIG_OPTION *const option, void *const user_data)
{
    M_DECLARED_ROW *const row = user_data;
    if (!M_PushCall(row, M_CB_FORMAT_VALUE, option, true)) {
        return nullptr;
    }
    if (!M_Call(row, M_CB_FORMAT_VALUE, 1)) {
        return nullptr;
    }
    const char *const text = lua_tostring(m_L, -1);
    Memory_FreePointer(&row->formatted);
    row->formatted = Memory_DupStr(text != nullptr ? text : "");
    lua_pop(m_L, 1);
    return row->formatted;
}

static bool M_IsAvailable(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return M_CallPredicate(
        user_data, M_CB_IS_AVAILABLE, option, 0, false, true);
}

static bool M_IsVisible(
    const CONFIG_OPTION *const option, void *const user_data)
{
    return M_CallPredicate(user_data, M_CB_IS_VISIBLE, option, 0, false, true);
}

static bool M_CanChangeValue(
    const CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    return M_CallPredicate(
        user_data, M_CB_CAN_CHANGE_VALUE, option, dir, true, true);
}

// False where the script did not take the press, so the dialogs move the
// setting themselves.
static bool M_RequestChangeValue(
    CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    return M_CallPredicate(
        user_data, M_CB_REQUEST_CHANGE_VALUE, option, dir, true, false);
}

// The one field of a callback table that is not a function.
static int32_t M_ReadDelta(
    lua_State *const L, const int32_t idx, const char *const name)
{
    lua_getfield(L, idx, name);
    const int32_t result = (int32_t)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
    return result;
}

static void M_ReadCallbacks(
    lua_State *const L, const int32_t idx, M_DECLARED_ROW *const row)
{
    for (int32_t i = 0; i < M_CB_COUNT; i++) {
        lua_getfield(L, idx, m_CallbackNames[i]);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            row->refs[i] = LUA_NOREF;
            continue;
        }
        luaL_checktype(L, -1, LUA_TFUNCTION);
        row->refs[i] = luaL_ref(L, LUA_REGISTRYINDEX);
    }
}

static void M_FreeRow(lua_State *const L, M_DECLARED_ROW *const row)
{
    UI_Settings_RemoveHandler(&row->handler);
    for (int32_t i = 0; i < M_CB_COUNT; i++) {
        if (L != nullptr && row->refs[i] != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, row->refs[i]);
        }
    }
    Memory_FreePointer(&row->formatted);
    Memory_FreePointer(&row->key);
    Memory_Free(row);
}

// trxc.settings.add_row(key, ui)
static int M_L_SettingsAddRow(lua_State *const L)
{
    const char *const key = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getfield(L, 2, "tab");
    const char *const tab_name = luaL_checkstring(L, -1);
    CONFIG_TAB tab;
    if (!UI_Settings_FindTab(tab_name, &tab)) {
        return luaL_error(L, "unknown settings tab: %s", tab_name);
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "before");
    lua_getfield(L, 2, "after");
    const char *const before = lua_isnil(L, -2) ? nullptr : lua_tostring(L, -2);
    const char *const after = lua_isnil(L, -1) ? nullptr : lua_tostring(L, -1);

    M_DECLARED_ROW *const row = Memory_Alloc(sizeof(M_DECLARED_ROW));
    row->key = Memory_DupStr(key);
    M_ReadCallbacks(L, 2, row);
    row->handler = (UI_SETTING_HANDLER) {
        .key = row->key,
        .user_data = row,
        .delta_slow = M_ReadDelta(L, 2, "delta_slow"),
        .delta_fast = M_ReadDelta(L, 2, "delta_fast"),
        .format_value =
            row->refs[M_CB_FORMAT_VALUE] != LUA_NOREF ? M_FormatValue : nullptr,
        .is_available =
            row->refs[M_CB_IS_AVAILABLE] != LUA_NOREF ? M_IsAvailable : nullptr,
        .is_visible =
            row->refs[M_CB_IS_VISIBLE] != LUA_NOREF ? M_IsVisible : nullptr,
        .can_change_value = row->refs[M_CB_CAN_CHANGE_VALUE] != LUA_NOREF
            ? M_CanChangeValue
            : nullptr,
        .request_change_value =
            row->refs[M_CB_REQUEST_CHANGE_VALUE] != LUA_NOREF
            ? M_RequestChangeValue
            : nullptr,
    };

    if (m_Rows == nullptr) {
        m_Rows = Vector_Create(sizeof(M_DECLARED_ROW *));
    }
    Vector_Add(m_Rows, &row);
    UI_Settings_AddHandler(&row->handler);
    UI_Settings_AddDeclaredRow(tab, key, before, after);
    lua_pop(L, 2);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "add_row", M_L_SettingsAddRow },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    m_L = L;
    LUA_RegisterModule(L, "settings", m_Module);
}

static void M_Shutdown(void)
{
    UI_SettingsLua_DropDeclaredRows();
    if (m_Rows != nullptr) {
        Vector_Free(m_Rows);
        m_Rows = nullptr;
    }
    m_L = nullptr;
}

void UI_SettingsLua_DropDeclaredRows(void)
{
    for (int32_t i = 0; m_Rows != nullptr && i < m_Rows->count; i++) {
        M_FreeRow(m_L, *(M_DECLARED_ROW **)Vector_Get(m_Rows, i));
    }
    if (m_Rows != nullptr) {
        Vector_Clear(m_Rows);
    }
    UI_Settings_DropDeclaredRows();
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
