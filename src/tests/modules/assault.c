#include <fakes/assault.h>
#include <fakes/game.h>
#include <fakes/sprites.h>
#include <fakes/ui_draw.h>
#include <harness/lua_surface.h>

#include <trx/config/registry.h>
#include <trx/config/types.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/ui.h>
#include <trx/game/objects/ids.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>
#include <trx/version.h>

#include <lauxlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Counts handler errors because dispatch continues after each error.
static int32_t m_ErrorCount = 0;

// Returns the number of logged handler errors.
static int M_FakeErrors(lua_State *const L)
{
    lua_pushinteger(L, m_ErrorCount);
    return 1;
}

// Applies fake timing changes on the next tick because timings are polled.
static int M_FakeTick(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_TICK);
    return 0;
}

static int M_FakeSetRun(lua_State *const L)
{
    const GYM_TRACK_TYPE track = (GYM_TRACK_TYPE)luaL_checkinteger(L, 1);
    FakeAssault_SetActiveTrack(track);
    FakeAssault_SetVisible(track, true);
    // The run clock is the level clock, so a level has to be up to hold one.
    FakeGame_SetCurrentLevel(0);
    FakeGame_SetRunTime((int32_t)luaL_checkinteger(L, 2));
    FakeAssault_SetTiming(
        track, FAKE_ASSAULT_TIMING_PENALTY, (int32_t)luaL_checkinteger(L, 3));
    FakeAssault_SetTiming(
        track, FAKE_ASSAULT_TIMING_TARGET_PENALTY,
        (int32_t)luaL_checkinteger(L, 4));
    FakeAssault_SetTiming(
        track, FAKE_ASSAULT_TIMING_PENALTY_TIMER,
        (int32_t)luaL_checkinteger(L, 5));
    return 0;
}

static int M_FakeSetLap(lua_State *const L)
{
    FakeAssault_SetTiming(
        GYM_TRACK_QUAD, FAKE_ASSAULT_TIMING_LAP_TIME,
        (int32_t)luaL_checkinteger(L, 1));
    FakeAssault_SetTiming(
        GYM_TRACK_QUAD, FAKE_ASSAULT_TIMING_LAP_TIMER,
        (int32_t)luaL_checkinteger(L, 2));
    return 0;
}

// Holds the level still, as the inventory ring and the pause screen do.
static int M_FakeSetPlaying(lua_State *const L)
{
    FakeGame_SetPlaying(lua_toboolean(L, 1));
    return 0;
}

// Returns the draw-region description and whether the widget tree is
// balanced.
static int M_FakeDrawRegions(lua_State *const L)
{
    m_ErrorCount = 0;
    FakeUIDraw_Forget();
    UI_BeginScene();
    const UI_NODE *const before = UI_GetCurrent();
    LUA_UI_SetDrawing(true);
    LUA_UI_DrawRegions();
    LUA_UI_SetDrawing(false);
    // Reports an unbalanced widget tree when a handler error interrupts
    // drawing.
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
    // TR3 shades the digits, which is the palette with two colors in it.
    g_TRVersion = 3;
    // Matches the assault course layout of ten digits, a colon, a full stop, a
    // T and an s.
    FakeSprites_Define(O_ASSAULT_DIGITS, 14, 20, 24);
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeDrawRegions);
    lua_setfield(L, -2, "draw_regions");
    lua_pushcfunction(L, M_FakeErrors);
    lua_setfield(L, -2, "errors");
    lua_pushcfunction(L, M_FakeTick);
    lua_setfield(L, -2, "tick");
    lua_pushcfunction(L, M_FakeSetRun);
    lua_setfield(L, -2, "set_run");
    lua_pushcfunction(L, M_FakeSetLap);
    lua_setfield(L, -2, "set_lap");
    lua_pushcfunction(L, M_FakeSetPlaying);
    lua_setfield(L, -2, "set_playing");
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
        .deps = { "config", "events", "signal", "assault", "catalog", "game",
                  "locale", "math", "ui.primitive", "ui.widgets", "ui.regions",
                  nullptr },
        .mod_script = "assault",
        // Loads the shipped module after sealing to match engine behaviour.
        .seal = true,
        .setup_extra = M_Setup,
        .push_fake = M_PushFake,
        .tests = "modules/assault",
    };
    return LuaSurface_Run(&test);
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

const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    return nullptr;
}

void UI_InitText(void)
{
}

void UI_ShutdownText(void)
{
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
