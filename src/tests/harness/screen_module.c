// Runs a shipped module that draws engine screens, against the real takeover
// state, a faked ring, faked saves and a recorded scene.

#include <harness/screen_module.h>

#include <fakes/game.h>
#include <fakes/input.h>
#include <fakes/items.h>
#include <fakes/savegame.h>
#include <fakes/ui_draw.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>
#include <harness/takeover_args.h>

#include <trx/config/registry.h>
#include <trx/config/types.h>
#include <trx/core/log.h>
#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/lua/registry.h>
#include <trx/game/shell/common.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/dialogs/takeover.h>
#include <trx/game/ui/fade.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Counts logged errors, because the dispatcher continues after them.
static int32_t m_ErrorCount = 0;

static SHELL_MOD m_Mods[] = {
    {
        .name = "base",
        .title = "Base",
        .mod_type = MOD_BASE_GAME,
        .is_available = true,
        .is_valid = true,
    },
};

static SHELL_ARGS m_Args = { .startup = { .mod = &m_Mods[0] } };

// Tells scripts when the fake ring's picked entry comes to rest and when it
// moves again, as the ring does once a tick.
static void M_ReportRingRest(void)
{
    static int32_t rest_frame = -1;
    const int32_t frame = FakeGame_GetRingRestFrame();
    if (frame != rest_frame) {
        rest_frame = frame;
        LUA_FireEventInt32(LUA_EVENT_RING_REST, frame);
    }
}

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    FakeInput_Reset();
    m_ErrorCount = 0;
    M_ReportRingRest();
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
    M_ReportRingRest();
    FakeInput_Tick();
    LUA_Tick();
    return 0;
}

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

// fake.scene() -> table of scheduled operations
static int M_FakeScene(lua_State *const L)
{
    FakeUIDraw_Forget();
    UI_SetPaintHook(LUA_UI_PaintRegions);
    UI_BeginScene();
    LUA_UI_SetReserving(true);
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        LUA_FireEventInt32(LUA_EVENT_UI_RESERVE, i);
    }
    LUA_UI_SetReserving(false);
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

// fake.set_description(object, text) - what the player reads on examining it.
static int M_FakeSetDescription(lua_State *const L)
{
    FakeItems_SetDescription(
        (OBJECT_ID)luaL_checkinteger(L, 1), luaL_checkstring(L, 2));
    return 0;
}

// fake.offer(screen, args?) -> bool - a screen the script takes holds the
// level still, as the engine's own screens do.
static int M_FakeOffer(lua_State *const L)
{
    M_ReportRingRest();
    const bool taken = UI_Takeover_Offer(
        (UI_TAKEOVER)luaL_checkinteger(L, 1), TakeoverArgs_Read(L, 2));
    if (taken) {
        FakeGame_SetPlaying(false);
    }
    lua_pushboolean(L, taken);
    return 1;
}

// fake.is_held(screen) -> bool
static int M_FakeIsHeld(lua_State *const L)
{
    lua_pushboolean(
        L, UI_Takeover_IsHeld((UI_TAKEOVER)luaL_checkinteger(L, 1)));
    return 1;
}

// fake.take_choice(screen) -> integer
static int M_FakeTakeChoice(lua_State *const L)
{
    lua_pushinteger(
        L, UI_Takeover_TakeChoice((UI_TAKEOVER)luaL_checkinteger(L, 1)));
    return 1;
}

// fake.start_game() - a level starts, as Game_Start reports it.
static int M_FakeStartGame(lua_State *const L)
{
    LUA_FireEventBool(LUA_EVENT_GAME_START, lua_toboolean(L, 1));
    return 0;
}

// fake.control() - the level runs one tick of its logic.
static int M_FakeControl(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_BEFORE_CONTROL);
    return 0;
}

// fake.start_title() - the title screen comes up.
static int M_FakeStartTitle(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_TITLE_START);
    return 0;
}

// fake.change_screen(cross_fades, fade_ticks, fade_hold) - the engine is
// about to start its next screen.
static int M_FakeChangeScreen(lua_State *const L)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_BOOL, .value.b = lua_toboolean(L, 1) },
        { .type = LUA_EVENT_ARG_INT32,
          .value.i32 = (int32_t)luaL_optinteger(L, 2, 0) },
        { .type = LUA_EVENT_ARG_INT32,
          .value.i32 = (int32_t)luaL_optinteger(L, 3, 0) },
    };
    LUA_FireEventEx(LUA_EVENT_SCREEN_CHANGE, args, 3);
    return 0;
}

// fake.holds_screen() -> boolean
static int M_FakeHoldsScreen(lua_State *const L)
{
    lua_pushboolean(L, UI_Fade_IsHoldingScreen());
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

// fake.release(screen)
static int M_FakeRelease(lua_State *const L)
{
    UI_Takeover_Release((UI_TAKEOVER)luaL_checkinteger(L, 1));
    FakeGame_SetPlaying(true);
    return 0;
}

// The game opens no debug library, so neither does this test.
static void M_Setup(lua_State *const L)
{
    Config_RegisterBuiltInOptions();
    lua_pushnil(L);
    lua_setglobal(L, "debug");
}

static void M_PushFake(lua_State *const L)
{
    FakeGame_PushLua(L);
    FakeSavegame_PushLua(L);
    FakeInput_PushLua(L);
    lua_pushcfunction(L, M_FakeErrors);
    lua_setfield(L, -2, "errors");
    lua_pushcfunction(L, M_FakeTick);
    lua_setfield(L, -2, "tick");
    lua_pushcfunction(L, M_FakeScene);
    lua_setfield(L, -2, "scene");
    lua_pushcfunction(L, M_FakeSceneLayers);
    lua_setfield(L, -2, "scene_layers");
    lua_pushcfunction(L, M_FakeOffer);
    lua_setfield(L, -2, "offer");
    lua_pushcfunction(L, M_FakeSetDescription);
    lua_setfield(L, -2, "set_description");
    lua_pushcfunction(L, M_FakeIsHeld);
    lua_setfield(L, -2, "is_held");
    lua_pushcfunction(L, M_FakeTakeChoice);
    lua_setfield(L, -2, "take_choice");
    lua_pushcfunction(L, M_FakeRelease);
    lua_setfield(L, -2, "release");
    lua_pushcfunction(L, M_FakeStartGame);
    lua_setfield(L, -2, "start_game");
    lua_pushcfunction(L, M_FakeStartTitle);
    lua_setfield(L, -2, "start_title");
    lua_pushcfunction(L, M_FakeControl);
    lua_setfield(L, -2, "control");
    lua_pushcfunction(L, M_FakeChangeScreen);
    lua_setfield(L, -2, "change_screen");
    lua_pushcfunction(L, M_FakeHoldsScreen);
    lua_setfield(L, -2, "holds_screen");
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
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

void Console_LogImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}

void Console_ShowImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
    if (level != LOG_LEVEL_ERROR) {
        return;
    }
    m_ErrorCount++;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

int32_t Shell_GetModCount(void)
{
    return 1;
}

const SHELL_MOD *Shell_GetMod(const int32_t index)
{
    return index == 0 ? &m_Mods[0] : nullptr;
}

const SHELL_ARGS *Shell_GetArgs(void)
{
    return &m_Args;
}

const SHELL_MOD *Shell_GetModByName(const char *const name)
{
    return strcmp(name, m_Mods[0].name) == 0 ? &m_Mods[0] : nullptr;
}

bool Shell_CanSwitchToMod(const SHELL_MOD *const mod)
{
    return false;
}

void Shell_RequestModSwitch(const char *const mod_name)
{
}

int ScreenModule_Run(const char *const tests)
{
    return ScreenModule_RunWith(tests, nullptr);
}

int ScreenModule_RunWith(
    const char *const tests, const char *const *const extra_deps)
{
    LUA_SURFACE_TEST test = {
        .module = "ui",
        .deps = { "config",         "events",    "signal",       "catalog",
                  "locale",         "math",      "input",        "sound",
                  "game",           "savegame",  "mod",          "inventory",
                  "inventory_ring", "overlay",   "ui.primitive", "ui.widgets",
                  "ui.regions",     "ui.layers", "ui.screens",   "ui.tween",
                  nullptr },
        .seal = true,
        .setup_extra = M_Setup,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .tests = tests,
    };
    int32_t count = 0;
    while (test.deps[count] != nullptr) {
        count++;
    }
    for (int32_t i = 0; extra_deps != nullptr && extra_deps[i] != nullptr;
         i++) {
        test.deps[count++] = extra_deps[i];
    }
    return LuaSurface_Run(&test);
}

void UI_InitText(void)
{
}

void UI_ShutdownText(void)
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
        *out_h = 15.0f * settings.scale;
    }
}

bool UI_Settings_IsBarLookPS1(const UI_BAR_TYPE type)
{
    return false;
}

const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    static const UI_BAR_THEME theme = {
        .kind = UI_BAR_THEME_PC_KIND,
        .basic_scale = 1.0f,
    };
    return &theme;
}
