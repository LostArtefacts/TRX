#include <trx/core/json/base.h>
#include <trx/core/json/parse.h>
#include <trx/core/json/types.h>
#include <trx/core/strings.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <string.h>

// Turning JSON text into Lua values. Where a file is, and whether a script may
// reach it, belongs to game/lua/capi/path.c.

// The source file location for a decoded value. A script can use it to report
// bad file data with the line to inspect. A value built by Lua carries none.
#define M_WHERE_KEY "__json_where"

#define M_MAX_DEPTH 100

static bool M_PushValue(
    lua_State *L, const JSON_VALUE *value, const char *path, int32_t depth);

// Marks the table on the stack with the line the value was written on. File
// text is parsed with location information, so every marked value is a
// JSON_VALUE_EX.
static void M_MarkWhere(
    lua_State *const L, const JSON_VALUE *const value, const char *const path)
{
    if (path == nullptr) {
        return;
    }
    const JSON_VALUE_EX *const ex = (const JSON_VALUE_EX *)value;
    lua_createtable(L, 0, 1);
    lua_pushstring(
        L,
        String_FormatStatic(
            "%s (line %d, col %d)", path, (int32_t)ex->line_no,
            (int32_t)ex->row_no));
    lua_setfield(L, -2, M_WHERE_KEY);
    lua_setmetatable(L, -2);
}

static void M_PushNumber(lua_State *const L, const JSON_VALUE *const value)
{
    const JSON_NUMBER *const number = JSON_ValueGetNumber(value);
    const bool is_float = number != nullptr
        && strcspn(number->number, ".eE") != strlen(number->number);
    if (is_float) {
        lua_pushnumber(L, JSON_ValueGetDouble(value, 0.0));
    } else {
        lua_pushinteger(L, JSON_ValueGetInt64(value, 0));
    }
}

static bool M_PushArray(
    lua_State *const L, const JSON_VALUE *const value,
    const JSON_ARRAY *const arr, const char *const path, const int32_t depth)
{
    lua_createtable(L, arr->length, 0);
    M_MarkWhere(L, value, path);
    int32_t idx = 1;
    for (const JSON_ARRAY_ELEMENT *elem = arr->start; elem != nullptr;
         elem = elem->next) {
        if (!M_PushValue(L, elem->value, path, depth + 1)) {
            return false;
        }
        lua_seti(L, -2, idx);
        idx++;
    }
    return true;
}

static bool M_PushObject(
    lua_State *const L, const JSON_VALUE *const value,
    const JSON_OBJECT *const obj, const char *const path, const int32_t depth)
{
    lua_createtable(L, 0, obj->length);
    M_MarkWhere(L, value, path);
    for (const JSON_OBJECT_ELEMENT *elem = obj->start; elem != nullptr;
         elem = elem->next) {
        if (!M_PushValue(L, elem->value, path, depth + 1)) {
            return false;
        }
        lua_setfield(L, -2, elem->name->string);
    }
    return true;
}

static bool M_PushValue(
    lua_State *const L, const JSON_VALUE *const value, const char *const path,
    const int32_t depth)
{
    if (depth >= M_MAX_DEPTH || lua_checkstack(L, 4) == 0) {
        return false;
    }
    switch ((JSON_TYPE)value->type) {
    case JSON_TYPE_STRING:
        lua_pushstring(L, JSON_ValueGetString(value, ""));
        break;
    case JSON_TYPE_NUMBER:
        M_PushNumber(L, value);
        break;
    case JSON_TYPE_OBJECT:
        return M_PushObject(L, value, JSON_ValueAsObject(value), path, depth);
    case JSON_TYPE_ARRAY:
        return M_PushArray(L, value, JSON_ValueAsArray(value), path, depth);
    case JSON_TYPE_TRUE:
        lua_pushboolean(L, true);
        break;
    case JSON_TYPE_FALSE:
        lua_pushboolean(L, false);
        break;
    case JSON_TYPE_NULL:
        lua_pushnil(L);
        break;
    }
    return true;
}

// Reads the text on the stack and pushes the decoded value. When `path` is set,
// errors report the file location and every table carries where it was written.
static int M_Decode(lua_State *const L, const char *const path)
{
    size_t size = 0;
    const char *const text = luaL_checklstring(L, 1, &size);
    const size_t flags = JSON_PARSE_FLAGS_ALLOW_JSON5
        | (path != nullptr ? JSON_PARSE_FLAGS_ALLOW_LOCATION_INFORMATION : 0);

    JSON_PARSE_RESULT pr;
    JSON_VALUE *const root =
        JSON_ParseEx(text, size, flags, nullptr, nullptr, &pr);
    if (root == nullptr) {
        return luaL_error(
            L, "%s%sline %d, col %d: %s", path != nullptr ? path : "",
            path != nullptr ? " " : "", (int32_t)pr.error_line_no,
            (int32_t)pr.error_row_no, JSON_GetErrorDescription(pr.error));
    }
    const bool ok = M_PushValue(L, root, path, 0);
    JSON_ValueFree(root);
    if (!ok) {
        return luaL_error(
            L, "%s%snesting goes deeper than %d", path != nullptr ? path : "",
            path != nullptr ? ": " : "", M_MAX_DEPTH);
    }
    return 1;
}

// trxc.json.decode(text) -> what the text holds
static int M_L_Decode(lua_State *const L)
{
    return M_Decode(L, nullptr);
}

// trxc.json.decode_from(text, path) -> what the text of a file holds
static int M_L_DecodeFrom(lua_State *const L)
{
    return M_Decode(L, luaL_checkstring(L, 2));
}

// trxc.json.where(value) -> text, or nil for a value no file holds
static int M_L_Where(lua_State *const L)
{
    if (!lua_istable(L, 1) || lua_getmetatable(L, 1) == 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_getfield(L, -1, M_WHERE_KEY);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "decode", M_L_Decode },
    { "decode_from", M_L_DecodeFrom },
    { "where", M_L_Where },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "json", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
