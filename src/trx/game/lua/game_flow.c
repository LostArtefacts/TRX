#include <trx/game/lua/game_flow.h>

static int32_t m_RunningCommands = 0;

void LUA_Console_BeginCommand(void)
{
    m_RunningCommands++;
}

void LUA_Console_EndCommand(void)
{
    m_RunningCommands--;
}

bool LUA_Console_IsRunningCommand(void)
{
    return m_RunningCommands > 0;
}
