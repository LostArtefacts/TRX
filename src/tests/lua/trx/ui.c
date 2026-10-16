// The interface surface. The assertions live in ui.lua; this stands up a
// scene for them to draw into.

#include <fakes/objects.h>
#include <fakes/ui_draw.h>
#include <harness/lua_surface.h>

#include <trx/game/lua/registry.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/ui/draw.h>
#include <trx/game/lua/utils/args.h>
#include <trx/config/option.h>
#include <trx/game/console/common.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <string.h>

// The viewport matches the canvas unless a test changes it.
static int32_t m_ViewportW = 640;
static int32_t m_ViewportH = 480;

// fake.paint(body) -> table of scheduled operations
//
// Runs body during the paint hook and returns the scheduled draw calls.
static int M_FakePaint(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    UI_BeginScene();
    FakeUIDraw_Forget();
    UI_BeginLayer(UI_LAYER_UNDER);
    LUA_UI_SetPainting(true);
    lua_pushvalue(L, 1);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_UI_SetPainting(false);
    UI_EndLayer(UI_LAYER_UNDER);
    if (status != LUA_OK) {
        LUA_UI_EndScene();
        return lua_error(L);
    }

    // Read before LUA_UI_EndScene paints the rest of the scene.
    const int32_t count = FakeUIDraw_GetCount();
    lua_createtable(L, count, 0);
    for (int32_t i = 0; i < count; i++) {
        lua_pushstring(L, FakeUIDraw_GetLine(i));
        lua_rawseti(L, -2, i + 1);
    }
    LUA_UI_EndScene();
    return 1;
}

// Fires the script paint event for the layer the scene is running.
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
//
// Runs a whole scene the way the engine runs one: every region is opened for
// the reserve event, then the scene is laid out, then each paint layer runs.
// The operations come back in the order they were scheduled, so a line drawn
// under the engine interface comes before one drawn over it.
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

// fake.set_viewport(w, h)
static int M_FakeSetViewport(lua_State *const L)
{
    m_ViewportW = (int32_t)luaL_checkinteger(L, 1);
    m_ViewportH = (int32_t)luaL_checkinteger(L, 2);
    return 0;
}

// fake.set_mesh_bounds(object, min_x, min_y, min_z, max_x, max_y, max_z)
static int M_FakeSetMeshBounds(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    FakeObjects_SetMeshBounds(
        object_id,
        (BOUNDS_16) {
            .min = {
                .x = (int16_t)luaL_checkinteger(L, 2),
                .y = (int16_t)luaL_checkinteger(L, 3),
                .z = (int16_t)luaL_checkinteger(L, 4),
            },
            .max = {
                .x = (int16_t)luaL_checkinteger(L, 5),
                .y = (int16_t)luaL_checkinteger(L, 6),
                .z = (int16_t)luaL_checkinteger(L, 7),
            },
        });
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
    LUA_Registry_DropLevelAll();
    return 0;
}

// fake.tick() - one engine tick.
static int M_FakeTick(lua_State *const L)
{
    LUA_Tick();
    return 0;
}

// fake.set_rate(rate) - how far the frame being drawn stands between the last
// two ticks.
static int M_FakeSetRate(lua_State *const L)
{
    FakeUIDraw_SetInterpolationRate(luaL_checknumber(L, 1));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeTick);
    lua_setfield(L, -2, "tick");
    lua_pushcfunction(L, M_FakeSetRate);
    lua_setfield(L, -2, "set_rate");
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_FakeSetViewport);
    lua_setfield(L, -2, "set_viewport");
    lua_pushcfunction(L, M_FakeSetMeshBounds);
    lua_setfield(L, -2, "set_mesh_bounds");
    lua_pushcfunction(L, M_FakePaint);
    lua_setfield(L, -2, "paint");
    lua_pushcfunction(L, M_FakeScene);
    lua_setfield(L, -2, "scene");
    lua_pushcfunction(L, M_FakeSceneLayers);
    lua_setfield(L, -2, "scene_layers");
}

// Stands in for the text renderer that ui/common.c links against and the
// bindings never reach.
void UI_InitText(void)
{
}

void UI_ShutdownText(void)
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
                  "ui.regions", "ui.tween", nullptr },
        .tests = "trx/ui",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}

void UI_Text_Draw(
    const char *const text, const float x, const float y,
    const UI_TEXT_SETTINGS settings)
{
}

// Follows the text scale that a fitted widget pushes, as the real one does.
void UI_Text_Measure(
    const char *const text, float *const out_w, float *const out_h,
    const UI_TEXT_SETTINGS settings)
{
    const float scale = settings.scale * UI_Scaler_GetContentScale();
    if (out_w != nullptr) {
        *out_w = (float)strlen(text) * 8.0f * scale;
    }
    if (out_h != nullptr) {
        *out_h = 16.0f * scale;
    }
}

bool UI_Settings_IsBarLookPS1(const UI_BAR_TYPE type)
{
    return type == UI_BAR_PROGRESS;
}

// A minimal theme is enough for geometry tests. The enemy's bar draws at half
// size, as the bars of some styles do.
const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    static UI_BAR_THEME theme = {
        .kind = UI_BAR_THEME_PC_KIND,
        .basic_scale = 1.0f,
    };
    static UI_BAR_THEME small_theme = {
        .kind = UI_BAR_THEME_PC_KIND,
        .basic_scale = 0.5f,
    };
    return type == UI_BAR_ENEMY_HP ? &small_theme : &theme;
}
