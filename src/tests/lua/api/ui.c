// The interface surface. The assertions live in ui.lua; this stands up a
// scene for them to draw into.

#include <harness/lua_surface.h>

#include <trx/game/lua/ui.h>
#include <trx/config/option.h>
#include <trx/game/console/common.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/elements/bar.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <string.h>

// The label as the scene records it. Standing the real one up would bring the
// text renderer, the fonts and the game strings with it, and the bindings are
// not tested for any of those.
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

static void M_PushFake(lua_State *const L)
{
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

void Console_LogEx(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}
CONFIG_OPTION *Config_FindOptionByMirror(const void *const mirror)
{
    return nullptr;
}
void Config_Option_Write(CONFIG_OPTION *const option, const TRX_VALUE *value)
{
}
void Config_Update(void)
{
}
// Reports a fixed canvas, where the real one measures the window.
int32_t Viewport_GetWidth(const VIEWPORT_SPACE space)
{
    return 640;
}

int32_t Viewport_GetHeight(const VIEWPORT_SPACE space)
{
    return 480;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "ui",
        .tests = "api/ui",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
