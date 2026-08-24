// Runs the shipped overlay module against a real scene and recorded draw calls.

#include <fakes/ui_draw.h>
#include <harness/lua_surface.h>

#include <trx/core/memory.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/ui.h>
#include <trx/config/registry.h>
#include <trx/config/types.h>
#include <trx/core/log.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>
#include <trx/game/ui/elements/bar.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Counts logged handler errors, because the dispatcher continues after them.
static int32_t m_ErrorCount = 0;

// fake.errors() -> integer
static int M_FakeErrors(lua_State *const L)
{
    lua_pushinteger(L, m_ErrorCount);
    return 1;
}

// fake.draw_regions() -> description, balanced
//
// Draws all overlay regions and returns the recorded scene.
static int M_FakeDrawRegions(lua_State *const L)
{
    m_ErrorCount = 0;
    UI_BeginScene();
    const UI_NODE *const before = UI_GetCurrent();
    LUA_UI_SetDrawing(true);
    LUA_UI_DrawRegions();
    LUA_UI_SetDrawing(false);
    // A handler error can leave the widget tree unbalanced.
    const bool balanced = UI_GetCurrent() == before;
    UI_EndScene();

    char *description = FakeUIDraw_Describe();
    lua_pushstring(L, description);
    Memory_FreePointer(&description);
    lua_pushboolean(L, balanced);
    return 2;
}

static void M_Setup(lua_State *const L)
{
    Config_RegisterBuiltInOptions();
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeDrawRegions);
    lua_setfield(L, -2, "draw_regions");
    lua_pushcfunction(L, M_FakeErrors);
    lua_setfield(L, -2, "errors");
}

CONFIG g_ConfigStorage = {};

void Log_StackTrace(void)
{
}

LOG_LEVEL Log_GetMinLevel(void)
{
    return LOG_LEVEL_WARNING;
}

void Log_Message(
    const LOG_LEVEL level, const char *const file, const int32_t line,
    const char *const func, const char *const fmt, ...)
{
    if (level == LOG_LEVEL_ERROR) {
        m_ErrorCount++;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "ui",
        .deps = { "config",       "events",     "signal",     "lara",
                  "items",        "objects",    "weapons",    "camera",
                  "rooms",        "catalog",    "locale",     "rules",
                  "inventory",    "game",       "cutscenes",  "overlay",
                  "ui.primitive", "ui.widgets", "ui.regions", nullptr },
        .mod_script = "overlay",
        // Load the shipped module after sealing, as the engine does.
        .seal = true,
        .setup_extra = M_Setup,
        .push_fake = M_PushFake,
        .tests = "modules/overlay",
    };
    return LuaSurface_Run(&test);
}

// Leaf widgets are recorded through fakes/ui_draw.c.
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

void Console_LogEx(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}

int32_t Viewport_GetWidth(const VIEWPORT_SPACE space)
{
    return 640;
}

int32_t Viewport_GetHeight(const VIEWPORT_SPACE space)
{
    return 480;
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

const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    return nullptr;
}
