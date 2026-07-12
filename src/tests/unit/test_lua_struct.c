#include "harness.h"

#include <trx/game/lua/struct.h>

#include <lauxlib.h>
#include <lualib.h>

// The struct bridge has no engine dependency, so the invariants the whole
// design rests on can be asserted against a synthetic type - no level, no
// items, no assets. These are the properties that make the public API opt-in
// rather than merely tidy: if any of them regress, a member the declarations
// deliberately withheld becomes reachable again.

typedef struct {
    int32_t visible;
    int32_t secret; // reachable by C, never declared to Lua
    int16_t locked; // read-only in C
    bool flag;
} WIDGET;

// clang-format off
static const FIELD_DESC M_WIDGET_FIELDS[] = {
    FIELD    (WIDGET, visible, FT_INT32),
    FIELD    (WIDGET, secret,  FT_INT32),
    FIELD    (WIDGET, flag,    FT_BOOL),
    FIELD_RO (WIDGET, locked,  FT_INT16),
};
// clang-format on

TYPE_DEFINE(WIDGET, M_WIDGET_FIELDS)

// A tiny pool so handles can be made stale on demand, the way Item_Kill does.
#define M_POOL_SIZE 4
static WIDGET m_Pool[M_POOL_SIZE];
static uint16_t m_Gen[M_POOL_SIZE];
static bool m_Dead[M_POOL_SIZE];

static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    if (ref->idx < 0 || ref->idx >= M_POOL_SIZE || m_Dead[ref->idx]) {
        return nullptr;
    }
    if (m_Gen[ref->idx] != ref->gen) {
        return nullptr;
    }
    return &m_Pool[ref->idx];
}

static int M_L_Reveal(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_WIDGET);
    const WIDGET *const w = LUA_Struct_Deref(L, ref);
    lua_pushinteger(L, w->visible);
    return 1;
}

// Exists in C, but no declaration exposes it. It must stay unreachable.
static int M_L_Backdoor(lua_State *const L)
{
    lua_pushstring(L, "the backdoor was reachable");
    return 1;
}

static const luaL_Reg M_METHODS[] = {
    { "reveal", M_L_Reveal },
    { "backdoor", M_L_Backdoor },
    { nullptr, nullptr },
};

static int M_L_GetWidget(lua_State *const L)
{
    const int32_t idx = luaL_checkinteger(L, 1);
    LUA_Struct_Push(L, &TYPE_WIDGET, M_Resolve, idx, m_Gen[idx]);
    return 1;
}

// Builds the environment a script would see, then applies the declaration the
// way data/scripting/*.lua does.
static lua_State *M_NewState(const char *const declaration)
{
    for (int32_t i = 0; i < M_POOL_SIZE; i++) {
        m_Pool[i] = (WIDGET) { .visible = 10 + i, .secret = 99, .locked = 7 };
        m_Gen[i] = 1;
        m_Dead[i] = false;
    }

    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);
    lua_newtable(L);
    lua_setglobal(L, "trxc");

    LUA_Struct_Register(L, &TYPE_WIDGET, M_METHODS);
    LUA_CreateStruct(L);

    lua_getglobal(L, "trxc");
    lua_pushcfunction(L, M_L_GetWidget);
    lua_setfield(L, -2, "get_widget");
    lua_pop(L, 1);

    if (declaration != nullptr && luaL_dostring(L, declaration) != LUA_OK) {
        TEST_FAIL("declaration failed: %s", lua_tostring(L, -1));
    }
    return L;
}

// The declaration every test starts from: `visible` and `flag` are public,
// `locked` is public but read-only, `reveal` is a method. `secret` and
// `backdoor` are deliberately withheld.
static const char DECL[] =
    "trxc.struct.expose_field('WIDGET', 'visible', 'visible', true)\n"
    "trxc.struct.expose_field('WIDGET', 'flag', 'flag', true)\n"
    "trxc.struct.expose_field('WIDGET', 'locked', 'locked', false)\n"
    "trxc.struct.expose_method('WIDGET', 'reveal', 'reveal')\n"
    "trxc.struct.expose_computed('WIDGET', 'doubled', function(w)\n"
    "  return w.visible * 2\n"
    "end)\n";

// Runs a chunk; returns true if it completed without error.
static bool M_Run(lua_State *const L, const char *const code)
{
    return luaL_dostring(L, code) == LUA_OK;
}

// Runs a chunk expected to raise. Returns the error message, or nullptr if it
// unexpectedly succeeded.
static const char *M_RunExpectingError(lua_State *const L, const char *code)
{
    if (luaL_dostring(L, code) == LUA_OK) {
        return nullptr;
    }
    return lua_tostring(L, -1);
}

TEST(a_declared_field_reads_and_writes)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local w = trxc.get_widget(0)\n"
        "assert(w.visible == 10, 'read')\n"
        "w.visible = 55\n"
        "assert(w.visible == 55, 'write')\n"
        "w.flag = true\n"
        "assert(w.flag == true, 'bool')\n"));
    CHECK_EQ_INT(m_Pool[0].visible, 55);
    lua_close(L);
}

// The property the whole design rests on: exposure is opt-in. A member the
// engine can reach but no declaration named is not merely hidden - it is
// absent.
TEST(an_undeclared_field_is_absent_not_hidden)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local w = trxc.get_widget(0)\n"
        "assert(w.secret == nil, 'an undeclared member leaked')\n"));

    // ...and it cannot be written either.
    const char *const err =
        M_RunExpectingError(L, "trxc.get_widget(0).secret = 1");
    CHECK_NOT_NULL(err);
    CHECK_EQ_INT(m_Pool[0].secret, 99); // untouched
    lua_close(L);
}

TEST(an_undeclared_method_is_unreachable)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local w = trxc.get_widget(0)\n"
        "assert(w.reveal ~= nil, 'declared method missing')\n"
        "assert(w:reveal() == 10, 'method call')\n"
        "assert(w.backdoor == nil, 'an undeclared method leaked')\n"));
    lua_close(L);
}

TEST(a_computed_member_is_invoked_on_access)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local w = trxc.get_widget(1)\n"
        "assert(w.doubled == 22, 'computed member')\n"));
    lua_close(L);
}

// C says a member must never be written; Lua may narrow that, but it must not
// be able to widen it.
TEST(a_c_readonly_member_cannot_be_declared_writable)
{
    lua_State *const L = M_NewState(nullptr);
    const char *const err = M_RunExpectingError(
        L, "trxc.struct.expose_field('WIDGET', 'locked', 'locked', true)");
    CHECK_NOT_NULL(err);
    lua_close(L);
}

TEST(writing_a_readonly_field_raises)
{
    lua_State *const L = M_NewState(DECL);
    const char *const err =
        M_RunExpectingError(L, "trxc.get_widget(0).locked = 1");
    CHECK_NOT_NULL(err);
    CHECK_EQ_INT(m_Pool[0].locked, 7); // unchanged
    lua_close(L);
}

TEST(writing_an_unknown_field_raises)
{
    lua_State *const L = M_NewState(DECL);
    CHECK_NOT_NULL(
        M_RunExpectingError(L, "trxc.get_widget(0).no_such_field = 1"));
    lua_close(L);
}

// Declaring a field that C cannot reach must fail loudly at declaration time,
// not silently produce a member that reads nil forever.
TEST(declaring_a_nonexistent_member_raises)
{
    lua_State *const L = M_NewState(nullptr);
    CHECK_NOT_NULL(M_RunExpectingError(
        L,
        "trxc.struct.expose_field('WIDGET', 'ghost', 'no_such_member', true)"));
    CHECK_NOT_NULL(M_RunExpectingError(
        L, "trxc.struct.expose_method('WIDGET', 'ghost', 'no_such_method')"));
    lua_close(L);
}

// A handle held across a kill must raise, not silently rebind to whatever
// reused the slot. This is what the generation counter buys.
TEST(a_stale_handle_raises_rather_than_rebinding)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(L, "held = trxc.get_widget(2)\nassert(held.visible == 12)\n"));

    // The slot is recycled by a different owner, as Item_Kill + Item_Create do.
    m_Gen[2]++;
    m_Pool[2].visible = 5000;

    const char *const err = M_RunExpectingError(L, "return held.visible");
    CHECK_NOT_NULL(err);
    if (err != nullptr) {
        CHECK(strstr(err, "stale") != nullptr);
    }
    lua_close(L);
}

TEST(a_handle_to_a_dead_slot_raises)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(L, "held = trxc.get_widget(3)\n"));
    m_Dead[3] = true;
    CHECK_NOT_NULL(M_RunExpectingError(L, "return held.visible"));
    lua_close(L);
}

// Without a protected metatable, getmetatable(handle).__raw_methods hands a
// script every C method the type could offer - including the ones no
// declaration exposed - which would make opt-in exposure a fiction.
TEST(the_metatable_is_protected)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local mt = getmetatable(trxc.get_widget(0))\n"
        "assert(type(mt) ~= 'table', 'the metatable is reachable')\n"
        "assert(mt == 'WIDGET', 'expected the __metatable sentinel')\n"));
    lua_close(L);
}

// pairs() must walk the declared surface, not the C table.
TEST(pairs_iterates_only_the_declared_fields)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local seen = {}\n"
        "for k, v in pairs(trxc.get_widget(0)) do seen[k] = v end\n"
        "assert(seen.visible == 10, 'declared field missing from pairs')\n"
        "assert(seen.locked == 7, 'declared field missing from pairs')\n"
        "assert(seen.secret == nil, 'pairs leaked an undeclared member')\n"));
    lua_close(L);
}

// trxc.struct.members reports what C can reach, so a script can be told which
// members nobody exposed. It must not become a way to reach them.
TEST(struct_members_reports_the_c_surface_for_diagnostics)
{
    lua_State *const L = M_NewState(DECL);
    CHECK(M_Run(
        L,
        "local names = {}\n"
        "for _, m in ipairs(trxc.struct.members('WIDGET')) do\n"
        "  names[m.name] = m\n"
        "end\n"
        "assert(names.secret ~= nil, 'members() should report undeclared "
        "ones')\n"
        "assert(names.locked.writable == false, 'locked is read-only in C')\n"
        "assert(names.visible.writable == true)\n"
        "assert(names.visible.type == 'INT32')\n"));
    lua_close(L);
}
