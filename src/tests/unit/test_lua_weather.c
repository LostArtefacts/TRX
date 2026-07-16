// The weather surface. The assertions live in src/tests/unit/lua/weather.lua.
//
// FX_Weather keeps the active weather in a single variable; the fake below is
// that variable and nothing more.

#include "lua_surface.h"

#include <trx/game/fx/weather.h>

static WEATHER_TYPE m_Weather = WEATHER_NONE;

void FX_Weather_SetWeather(const WEATHER_TYPE weather_type)
{
    m_Weather = weather_type;
}

WEATHER_TYPE FX_Weather_GetWeather(void)
{
    return m_Weather;
}

static int M_FakeReset(lua_State *const L)
{
    m_Weather = WEATHER_NONE;
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "weather",
        .tests = "weather",
        .seal = true,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
