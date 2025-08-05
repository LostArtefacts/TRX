#include "game/output/state.h"

#include "utils.h"

static int32_t m_FogStart = 0;

int32_t Output_GetFogStart(void)
{
    return MIN(m_FogStart, Output_GetFogEnd());
}

void Output_SetFogStart(const int32_t dist)
{
    m_FogStart = dist;
}
