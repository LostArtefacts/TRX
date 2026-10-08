// Stands up the layer stack over a recorded scene and a faked input. The
// assertions live in layers.lua.

#include <fakes/input.h>
#include <fakes/ui_draw.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/lua/registry.h>
#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/ui.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/elements/bar.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <string.h>

// Counts the handler errors that the dispatcher reports and then continues
// past.
static int32_t m_ErrorCount = 0;

// The viewport matches the canvas unless a test changes it.
static int32_t m_ViewportW = 640;
static int32_t m_ViewportH = 480;

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    FakeInput_Reset();
    m_ErrorCount = 0;
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

// fake.tick()
static int M_FakeTick(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_TICK);
    return 0;
}

static void M_PaintLayer(const UI_PAINT_LAYER layer)
{
    LUA_UI_SetPainting(true);
    LUA_UI_SetPaintDepth(layer == UI_PAINT_LAYER_OVER ? 0 : LUA_UI_UNDER_DEPTH);
    LUA_FireEvent(
        layer == UI_PAINT_LAYER_OVER ? LUA_EVENT_UI_PAINT_OVER
                                     : LUA_EVENT_UI_PAINT);
    LUA_UI_SetPainting(false);
}

// fake.scene() -> table of scheduled operations
//
// Runs a whole scene as the engine runs one: the draw event for every region,
// the layout, and then each paint pass.
static int M_FakeScene(lua_State *const L)
{
    FakeUIDraw_Forget();
    UI_SetPaintHook(M_PaintLayer);
    UI_BeginScene();
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        UI_BeginRegion((UI_REGION)i);
        LUA_UI_SetDrawing(true);
        LUA_FireEventInt32(LUA_EVENT_UI_DRAW, i);
        LUA_UI_SetDrawing(false);
        UI_EndRegion();
    }
    UI_EndScene();
    UI_SetPaintHook(nullptr);

    const int32_t count = FakeUIDraw_GetCount();
    lua_createtable(L, count, 0);
    for (int32_t i = 0; i < count; i++) {
        lua_pushstring(L, FakeUIDraw_GetLine(i));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
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
    FakeInput_PushLua(L);
    lua_pushcfunction(L, M_FakeErrors);
    lua_setfield(L, -2, "errors");
    lua_pushcfunction(L, M_FakeTick);
    lua_setfield(L, -2, "tick");
    lua_pushcfunction(L, M_FakeScene);
    lua_setfield(L, -2, "scene");
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
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

void UI_Label(const char *const text)
{
}

void UI_LabelEx(const char *const text, const UI_LABEL_SETTINGS settings)
{
}

void UI_Label_MeasureEx(
    const char *const text, float *const out_w, float *const out_h,
    const UI_LABEL_SETTINGS settings)
{
    if (out_w != nullptr) {
        *out_w = (float)strlen(text) * 8.0f * settings.scale;
    }
    if (out_h != nullptr) {
        *out_h = 16.0f * settings.scale;
    }
}

void UI_Bar(const UI_BAR_SETTINGS settings)
{
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

const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    static UI_BAR_THEME theme = {
        .kind = UI_BAR_THEME_PC_KIND,
        .basic_scale = 1.0f,
    };
    return &theme;
}
