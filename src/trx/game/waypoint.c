#include <trx/game/waypoint.h>

static struct {
    int32_t num;
    int32_t highest;
    int32_t pad;
} m_State = {
    .num = WAYPOINT_NONE,
    .highest = WAYPOINT_NONE,
    .pad = WAYPOINT_PAD_NONE,
};

int32_t Waypoint_Get(void)
{
    return m_State.num;
}

void Waypoint_Set(const int32_t num)
{
    m_State.num = num;
    if (num > m_State.highest) {
        m_State.highest = num;
    }
}

int32_t Waypoint_GetHighest(void)
{
    return m_State.highest;
}

void Waypoint_SetHighest(const int32_t num)
{
    m_State.highest = num;
}

int32_t Waypoint_GetPad(void)
{
    return m_State.pad;
}

void Waypoint_SetPad(const int32_t num)
{
    m_State.pad = num;
    m_State.num = num;
}

void Waypoint_ClearPad(void)
{
    m_State.pad = WAYPOINT_PAD_NONE;
}

void Waypoint_Reset(void)
{
    m_State.num = WAYPOINT_NONE;
    m_State.highest = WAYPOINT_NONE;
    m_State.pad = WAYPOINT_PAD_NONE;
}
