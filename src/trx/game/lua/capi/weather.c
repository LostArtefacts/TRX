#include <trx/game/fx/weather.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

// trxc.weather.set(type)
static int M_L_WeatherSet(lua_State *const L)
{
    const lua_Integer type = luaL_checkinteger(L, 1);
    if (type < WEATHER_NONE || type > WEATHER_SNOW) {
        return luaL_error(L, "invalid weather type");
    }
    FX_Weather_SetWeather((WEATHER_TYPE)type);
    return 0;
}

// trxc.weather.get() -> int
static int M_L_WeatherGet(lua_State *const L)
{
    lua_pushinteger(L, FX_Weather_GetWeather());
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "set", M_L_WeatherSet },
    { "get", M_L_WeatherGet },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "weather", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
