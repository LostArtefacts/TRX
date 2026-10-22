// Stands up the layer stack over a recorded scene and a faked input. The
// assertions live in layers.lua.

#include <fakes/game.h>
#include <fakes/input.h>
#include <fakes/ui_draw.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/lua/registry.h>
#include <trx/config.h>
#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Counts the handler errors that the dispatcher reports and then continues
// past.
static int32_t m_ErrorCount = 0;

// The message of the last handler error that the dispatcher reported.
static char m_LastError[512] = { 0 };

// The viewport matches the canvas unless a test changes it.
static int32_t m_ViewportW = 640;
static int32_t m_ViewportH = 480;

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    FakeInput_Reset();
    FakeGame_SetPlaying(false);
    m_ErrorCount = 0;
    m_LastError[0] = '\0';
    m_ViewportW = 640;
    m_ViewportH = 480;
    return 0;
}

// fake.errors() -> integer
static int M_FakeErrors(lua_State *const L)
{
    lua_pushinteger(L, m_ErrorCount);
    return 1;
}

// fake.last_error() -> string
static int M_FakeLastError(lua_State *const L)
{
    lua_pushstring(L, m_LastError);
    return 1;
}

// fake.tick()
static int M_FakeTick(lua_State *const L)
{
    FakeInput_Tick();
    LUA_Tick();
    return 0;
}

// fake.scene() -> table of scheduled operations
//
// fake.scene_layers() -> the layer of each op of the last scene, by name
static int M_FakeSceneLayers(lua_State *const L)
{
    const int32_t count = FakeUIDraw_GetCount();
    lua_createtable(L, count, 0);
    for (int32_t i = 0; i < count; i++) {
        lua_pushstring(L, FakeUIDraw_GetLayerName(i));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// Runs a whole scene as the engine runs one: the reserve event for every
// region, the layout, and then each paint pass.
static int M_FakeScene(lua_State *const L)
{
    FakeUIDraw_Forget();
    UI_BeginScene();
    LUA_UI_SetReserving(true);
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        LUA_FireEventInt32(LUA_EVENT_UI_RESERVE, i);
    }
    LUA_UI_SetReserving(false);
    LUA_UI_EndScene();

    const int32_t count = FakeUIDraw_GetCount();
    lua_createtable(L, count, 0);
    for (int32_t i = 0; i < count; i++) {
        lua_pushstring(L, FakeUIDraw_GetLine(i));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// fake.set_fade_effects(on) - the fade effects setting, which fade layers
// leave to the scripts that move them.
static int M_FakeSetFadeEffects(lua_State *const L)
{
    g_ConfigStorage.visuals.enable_fade_effects = lua_toboolean(L, 1);
    return 0;
}

// fake.as_level_script(fn) - runs fn as a level script.
static int M_FakeAsLevelScript(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

// fake.end_level() - the unload event, and then the level's listeners go.
static int M_FakeEndLevel(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    LUA_Registry_DropLevelAll();
    return 0;
}

// fake.set_playing(playing) - whether a level is being played.
static int M_FakeSetPlaying(lua_State *const L)
{
    FakeGame_SetPlaying(lua_toboolean(L, 1));
    return 0;
}

// fake.set_viewport(w, h)
static int M_FakeSetViewport(lua_State *const L)
{
    m_ViewportW = (int32_t)luaL_checkinteger(L, 1);
    m_ViewportH = (int32_t)luaL_checkinteger(L, 2);
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetViewport);
    lua_setfield(L, -2, "set_viewport");
    lua_pushcfunction(L, M_FakeSetPlaying);
    lua_setfield(L, -2, "set_playing");
    FakeInput_PushLua(L);
    lua_pushcfunction(L, M_FakeErrors);
    lua_setfield(L, -2, "errors");
    lua_pushcfunction(L, M_FakeLastError);
    lua_setfield(L, -2, "last_error");
    lua_pushcfunction(L, M_FakeTick);
    lua_setfield(L, -2, "tick");
    lua_pushcfunction(L, M_FakeScene);
    lua_setfield(L, -2, "scene");
    lua_pushcfunction(L, M_FakeSceneLayers);
    lua_setfield(L, -2, "scene_layers");
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeSetFadeEffects);
    lua_setfield(L, -2, "set_fade_effects");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
}

// The game opens no debug library, so neither does this test.
static void M_Setup(lua_State *const L)
{
    lua_pushnil(L);
    lua_setglobal(L, "debug");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "ui.layers",
        .deps = { "signal", "math", "events", "input", "ui.primitive",
                  "ui.widgets", "ui.regions", nullptr },
        .tests = "trx/ui/layers",
        .seal = true,
        .setup_extra = M_Setup,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
    };
    return LuaSurface_Run(&test);
}

void UI_InitText(void)
{
}

void UI_ShutdownText(void)
{
}

void Console_LogImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}

void Console_ShowImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
    if (level == LOG_LEVEL_ERROR) {
        m_ErrorCount++;
        va_list va;
        va_start(va, fmt);
        vsnprintf(m_LastError, sizeof(m_LastError), fmt, va);
        va_end(va);
    }
}

int32_t Viewport_GetWidth(const VIEWPORT_SPACE space)
{
    return m_ViewportW;
}

int32_t Viewport_GetHeight(const VIEWPORT_SPACE space)
{
    return m_ViewportH;
}

void UI_Text_Draw(
    const char *const text, const float x, const float y,
    const UI_TEXT_SETTINGS settings)
{
    FakeUIDraw_Record(String_FormatStatic(
        "text x=%d y=%d z=%d text=%s", (int32_t)x, (int32_t)y, settings.z,
        text));
}

void UI_Text_Measure(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings)
{
    if (out_w != nullptr) {
        *out_w = (float)strlen(text) * 8.0f * settings.scale;
    }
    if (out_h != nullptr) {
        *out_h = 16.0f * settings.scale;
    }
}

float UI_Text_GetBaseline(const UI_TEXT_SETTINGS settings)
{
    return 16.0f * settings.scale;
}
