#include "specific/output.h"
#include "mod.h"
#include "util.h"

void __cdecl S_DrawHealthBar(int32_t percent)
{
    TR1MRenderBar(percent, 100, TR1M_BAR_LARA_HEALTH);
}

void __cdecl S_DrawAirBar(int32_t percent)
{
    TR1MRenderBar(percent, 100, TR1M_BAR_LARA_AIR);
}

void TR1MInjectSpecificOutput()
{
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
}
