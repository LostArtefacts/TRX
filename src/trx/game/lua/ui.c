#include <trx/game/lua/ui.h>

#include <trx/core/subsystem.h>
#include <trx/game/lua/events.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/regions.h>

static bool m_HideNextFrame = false;

static void M_Init(void)
{
    UI_SetPaintHook(LUA_UI_PaintRegions);
}

void LUA_UI_PaintRegions(void)
{
    LUA_UI_SetPainting(true);
    LUA_FireEvent(LUA_EVENT_UI_PAINT);
    LUA_UI_SetPainting(false);
}

void LUA_UI_HideNextFrame(void)
{
    m_HideNextFrame = true;
}

void LUA_UI_DrawRegions(void)
{
    if (m_HideNextFrame) {
        m_HideNextFrame = false;
        return;
    }
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        UI_BeginRegion((UI_REGION)i);
        LUA_UI_SetDrawing(true);
        LUA_FireEventInt32(LUA_EVENT_UI_DRAW, i);
        LUA_UI_SetDrawing(false);
        UI_EndRegion();
    }
}

REGISTER_SUBSYSTEM(.init = M_Init)
