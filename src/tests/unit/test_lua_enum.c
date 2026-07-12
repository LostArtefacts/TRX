#include "harness.h"

#include <trx/core/enum_map.h>

#include <lauxlib.h>
#include <lualib.h>

// The Lua enum bridge over ENUM_MAP, exercised against a synthetic enum - no
// level, no items, no assets.
//
// The property that matters: an enum's names and values are written once, in C.
// Lua reflects them rather than restating them, so trx.items.Status cannot
// drift from ITEM_STATUS, and the generated reference cannot document a
// constant the engine does not have. These tests pin the reflection that
// guarantees it.

extern void LUA_CreateEnum(lua_State *L);

typedef enum {
    M_WIDGET_OFF = 0,
    M_WIDGET_ON = 1,
    M_WIDGET_BROKEN = 7,
} M_WIDGET_STATE;

static __attribute__((constructor)) void M_Init(void)
{
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_OFF, "off");
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_ON, "on");
    ENUM_MAP(M_WIDGET_STATE, M_WIDGET_BROKEN, "broken");
}

static lua_State *M_NewState(void)
{
    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);
    lua_newtable(L);
    lua_setglobal(L, "trxc");
    LUA_CreateEnum(L);
    return L;
}

static bool M_Run(lua_State *const L, const char *const code)
{
    return luaL_dostring(L, code) == LUA_OK;
}

static const char *M_RunExpectingError(lua_State *const L, const char *code)
{
    if (luaL_dostring(L, code) == LUA_OK) {
        return nullptr;
    }
    return lua_tostring(L, -1);
}

TEST(values_reflects_every_constant_with_its_number)
{
    lua_State *const L = M_NewState();
    // Keyed by name so the assertions do not depend on reflection order, which
    // the underlying hash does not promise.
    CHECK(M_Run(
        L,
        "local by_name = {}\n"
        "for _, c in ipairs(trxc.enum.values('M_WIDGET_STATE')) do\n"
        "  by_name[c.name] = c.value\n"
        "end\n"
        "assert(by_name.OFF == 0, 'OFF')\n"
        "assert(by_name.ON == 1, 'ON')\n"
        // Not 2: the numbers come from C, so a gap in the enum survives.
        "assert(by_name.BROKEN == 7, 'BROKEN')\n"
        "local n = 0\n"
        "for _ in pairs(by_name) do n = n + 1 end\n"
        "assert(n == 3, 'expected exactly 3 constants, got ' .. n)\n"));
    lua_close(L);
}

// The public key is the uppercased ENUM_MAP string, which is what preserves
// trx.items.Status.ACTIVE while C keeps its lowercase "active".
TEST(values_uppercases_the_public_name)
{
    lua_State *const L = M_NewState();
    CHECK(M_Run(
        L,
        "for _, c in ipairs(trxc.enum.values('M_WIDGET_STATE')) do\n"
        "  assert(c.name == c.name:upper(), c.name .. ' is not uppercase')\n"
        "end\n"));
    lua_close(L);
}

// A misspelled type name must raise, not hand back an empty table: an empty
// table would sail through the declaration and document the enum as having no
// constants at all.
TEST(an_unknown_enum_raises)
{
    lua_State *const L = M_NewState();
    const char *const err =
        M_RunExpectingError(L, "return trxc.enum.values('NO_SUCH_ENUM')\n");
    CHECK_NOT_NULL(err);
    if (err != nullptr) {
        CHECK(strstr(err, "unknown enum") != nullptr);
    }
    lua_close(L);
}
