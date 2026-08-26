// The interface surface. The assertions live in ui.lua; this stands up a
// scene for them to draw into.

#include <fakes/ui_draw.h>
#include <harness/lua_surface.h>

#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/ui.h>
#include <trx/config/option.h>
#include <trx/game/console/common.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/elements/bar.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <string.h>

// The label as the scene records it. Standing the real one up would bring the
// text renderer, the fonts and the game strings with it, and the bindings are
// not tested for any of those.
// The viewport matches the canvas unless a test changes it.
static int32_t m_ViewportW = 640;
static int32_t m_ViewportH = 480;

static char m_LastLabel[64] = {};
static int32_t m_LastBarValue = -1;

// fake.last_label() -> string or nil
static int M_FakeLastLabel(lua_State *const L)
{
    if (m_LastLabel[0] == '\0') {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, m_LastLabel);
    }
    return 1;
}

// fake.last_bar() -> integer or nil
static int M_FakeLastBar(lua_State *const L)
{
    if (m_LastBarValue < 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, m_LastBarValue);
    }
    return 1;
}

// fake.draw(body) -> balanced, error
//
// Opens a scene the way the engine does around the draw event, runs the body
// inside it, and answers whether the node stack came back to where it started.
// Anything else means a widget was left open for the next frame to draw into.
static int M_FakeDraw(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);

    m_LastLabel[0] = '\0';
    m_LastBarValue = -1;
    UI_BeginScene();
    const UI_NODE *const before = UI_GetCurrent();
    LUA_UI_SetDrawing(true);
    lua_pushvalue(L, 1);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_UI_SetDrawing(false);
    const bool balanced = UI_GetCurrent() == before;
    UI_EndScene();

    lua_pushboolean(L, balanced);
    if (status != LUA_OK) {
        lua_insert(L, -2);
        return 2;
    }
    return 1;
}

// fake.paint(body) -> table of scheduled operations
//
// Runs body during the paint hook and returns the scheduled draw calls.
static int M_FakePaint(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    UI_BeginScene();
    FakeUIDraw_Forget();
    LUA_UI_SetPainting(true);
    lua_pushvalue(L, 1);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_UI_SetPainting(false);
    if (status != LUA_OK) {
        UI_EndScene();
        return lua_error(L);
    }

    // Read before UI_EndScene clears the scene state.
    const int32_t count = FakeUIDraw_GetCount();
    lua_createtable(L, count, 0);
    for (int32_t i = 0; i < count; i++) {
        lua_pushstring(L, FakeUIDraw_GetLine(i));
        lua_rawseti(L, -2, i + 1);
    }
    UI_EndScene();
    return 1;
}

// fake.set_viewport(w, h)
static int M_FakeSetViewport(lua_State *const L)
{
    m_ViewportW = (int32_t)luaL_checkinteger(L, 1);
    m_ViewportH = (int32_t)luaL_checkinteger(L, 2);
    return 0;
}

// fake.as_level_script(fn) - run fn as a level script rather than a global one,
// so the widgets it places are the level's.
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

// fake.end_level() - what the engine does when a level ends, in the order it
// does it: the script hears the unload, and then its listeners go.
static int M_FakeEndLevel(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    LUA_ClearLevelListeners();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_FakeSetViewport);
    lua_setfield(L, -2, "set_viewport");
    lua_pushcfunction(L, M_FakePaint);
    lua_setfield(L, -2, "paint");
    lua_pushcfunction(L, M_FakeDraw);
    lua_setfield(L, -2, "draw");
    lua_pushcfunction(L, M_FakeLastLabel);
    lua_setfield(L, -2, "last_label");
    lua_pushcfunction(L, M_FakeLastBar);
    lua_setfield(L, -2, "last_bar");
}

void UI_Label(const char *const text)
{
    snprintf(m_LastLabel, sizeof(m_LastLabel), "%s", text);
}

void UI_LabelEx(const char *const text, const UI_LABEL_SETTINGS settings)
{
    UI_Label(text);
}

// Works a size out from the length of the text, so that a measurement can be
// checked without the fonts the real one loads.
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
    m_LastBarValue = settings.value;
}

// Stands in for what ui/common.c links against and the bindings never reach:
// the settings a toggle would write, the console it would report to, and the
// text renderer.
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
int32_t Viewport_GetWidth(const VIEWPORT_SPACE space)
{
    return m_ViewportW;
}

int32_t Viewport_GetHeight(const VIEWPORT_SPACE space)
{
    return m_ViewportH;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "ui",
        // ui.widgets is loaded explicitly because trx.ui does not require it.
        .deps = { "signal", "math", "events", "ui.primitive", "ui.widgets",
                  "ui.regions", nullptr },
        .tests = "api/ui",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}

void UI_Text_Draw(
    const char *const text, const float x, const float y,
    const UI_TEXT_SETTINGS settings)
{
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

// A minimal theme is enough for geometry tests.
const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    static UI_BAR_THEME theme = {
        .kind = UI_BAR_THEME_PC_KIND,
        .basic_scale = 1.0f,
    };
    return &theme;
}
