#include <harness/fake_calls.h>

#include <trx/core/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A suite runs a handful of calls per case and the log is cleared between them,
// so these are far above what any case reaches. Overflowing drops a call, which
// would read as a call that never happened, so it stops instead.
#define FAKE_MAX_CALLS 256
#define FAKE_MAX_ARGS 8
#define FAKE_MAX_RESETS 32

typedef struct {
    const char *name;
    int32_t arg_count;
    FAKE_ARG args[FAKE_MAX_ARGS];
    // Owned copies behind the borrowed as_str of a string argument.
    char *strings[FAKE_MAX_ARGS];
} FAKE_CALL;

static FAKE_CALL m_Calls[FAKE_MAX_CALLS];
static int32_t m_CallCount;
static void (*m_Resets[FAKE_MAX_RESETS])(void);
static int32_t m_ResetCount;

static void M_Fail(const char *what, int32_t limit);
static void M_PushValue(lua_State *L, const char *name, const TRX_VALUE *value);
static void M_PushArgs(lua_State *L, const FAKE_CALL *call);

static void M_Fail(const char *const what, const int32_t limit)
{
    fprintf(stderr, "more than %d %s recorded; raise the limit\n", limit, what);
    abort();
}

static void M_PushValue(
    lua_State *const L, const char *const name, const TRX_VALUE *const value)
{
    switch (value->type) {
    case TVT_BOOL:
        lua_pushboolean(L, value->as_bool);
        return;
    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
    case TVT_ENUM:
        lua_pushinteger(L, (lua_Integer)value->as_int);
        return;
    case TVT_FLOAT:
    case TVT_DOUBLE:
        lua_pushnumber(L, value->as_num);
        return;
    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        lua_pushstring(L, value->as_str);
        return;
    case TVT_XYZ_16:
    case TVT_XYZ_32:
    case TVT_RGB_888:
        break;
    }
    // Reached only by an argument of a type nothing records yet. Failing here
    // beats pushing nil and letting the assertion read as a pass.
    luaL_error(L, "cannot record %s as %s", name, Value_TypeName(value->type));
}

void FakeCalls_Record(const char *const name, const FAKE_ARG *args)
{
    if (m_CallCount >= FAKE_MAX_CALLS) {
        M_Fail("calls", FAKE_MAX_CALLS);
    }
    FAKE_CALL *const call = &m_Calls[m_CallCount++];
    call->name = name;
    call->arg_count = 0;
    for (; args->name != nullptr; args++) {
        if (call->arg_count >= FAKE_MAX_ARGS) {
            M_Fail("arguments to one call", FAKE_MAX_ARGS);
        }
        const int32_t idx = call->arg_count++;
        call->args[idx] = *args;
        if (args->value.type == TVT_STRING && args->value.as_str != nullptr) {
            call->strings[idx] = strdup(args->value.as_str);
            call->args[idx].value.as_str = call->strings[idx];
        }
    }
}

void FakeCalls_Reset(void)
{
    for (int32_t i = 0; i < m_CallCount; i++) {
        for (int32_t j = 0; j < m_Calls[i].arg_count; j++) {
            free(m_Calls[i].strings[j]);
        }
        m_Calls[i] = (FAKE_CALL) {};
    }
    m_CallCount = 0;
    for (int32_t i = 0; i < m_ResetCount; i++) {
        m_Resets[i]();
    }
}

void FakeCalls_OnReset(void (*const func)(void))
{
    if (m_ResetCount >= FAKE_MAX_RESETS) {
        M_Fail("reset hooks", FAKE_MAX_RESETS);
    }
    m_Resets[m_ResetCount++] = func;
}

// The arguments of one call, as a table, with its name under `name`.
static void M_PushArgs(lua_State *const L, const FAKE_CALL *const call)
{
    lua_pushstring(L, call->name);
    lua_setfield(L, -2, "name");
    for (int32_t i = 0; i < call->arg_count; i++) {
        M_PushValue(L, call->args[i].name, &call->args[i].value);
        lua_setfield(L, -2, call->args[i].name);
    }
}

// Answers a count of zero for a call that never happened, so a test can say so
// without the group having to exist.
static int M_GroupIndex(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "count");
    return 1;
}

int FakeCalls_Push(lua_State *const L)
{
    lua_newtable(L);

    for (int32_t i = 0; i < m_CallCount; i++) {
        const FAKE_CALL *const call = &m_Calls[i];

        // The group for this call name, holding its count and the arguments of
        // the most recent one.
        lua_getfield(L, -1, call->name);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushinteger(L, 0);
            lua_setfield(L, -2, "count");
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, call->name);
        }
        lua_getfield(L, -1, "count");
        const lua_Integer count = lua_tointeger(L, -1);
        lua_pop(L, 1);
        lua_pushinteger(L, count + 1);
        lua_setfield(L, -2, "count");
        M_PushArgs(L, call);
        lua_pop(L, 1);

        // The same call in the ordered sequence.
        lua_newtable(L);
        M_PushArgs(L, call);
        lua_rawseti(L, -2, i + 1);
    }

    lua_newtable(L);
    lua_pushcfunction(L, M_GroupIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    return 1;
}
