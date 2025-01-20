#include "gfx/gl/track.h"

GFX_METRICS g_GFX_Metrics;

void GFX_Track_Reset(void)
{
    g_GFX_Metrics = (GFX_METRICS) { 0 };
}

GFX_METRICS GFX_Track_GetMetrics(void)
{
    return g_GFX_Metrics;
}
