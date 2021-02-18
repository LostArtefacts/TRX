#include "specific/output.h"
#include "mod.h"
#include "util.h"

void __cdecl S_DrawHealthBar(int32_t percent)
{
    Tomb1MRenderBar(percent, 100, Tomb1M_BAR_LARA_HEALTH);
}

void __cdecl S_DrawAirBar(int32_t percent)
{
    Tomb1MRenderBar(percent, 100, Tomb1M_BAR_LARA_AIR);
}

void Tomb1MInjectSpecificOutput()
{
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
}
