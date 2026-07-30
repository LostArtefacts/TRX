// The weather surface. The assertions live in weather.lua.
//
// FX_Weather keeps the active weather in a single variable; the fake below is
// that variable and nothing more.

#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

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
    FakeCalls_Reset();
    m_Weather = WEATHER_NONE;
    return 0;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "weather",
        .tests = "api/weather",
        .seal = true,
    };
    return LuaSurface_Run(&test);
}
