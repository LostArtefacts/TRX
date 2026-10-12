#include <trx/game/lua/events/ui.h>

#include <trx/core/subsystem.h>
#include <trx/game/lua/events.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/regions.h>

static bool m_HideNextFrame = false;

static void M_Init(void)
{
    UI_SetPaintHook(LUA_UI_PaintRegions);
}

void LUA_UI_PaintRegions(const UI_LAYER layer)
{
    LUA_EVENT_TYPE event;
    switch (layer) {
    case UI_LAYER_UNDER:
        event = LUA_EVENT_UI_PAINT;
        break;
    case UI_LAYER_OVER:
        event = LUA_EVENT_UI_PAINT_OVER;
        break;
    case UI_LAYER_CONSOLE:
        event = LUA_EVENT_UI_PAINT_CONSOLE;
        break;
    default:
        return;
    }
    LUA_UI_SetPainting(true);
    LUA_UI_SetPaintDepth(layer == UI_LAYER_UNDER ? LUA_UI_UNDER_DEPTH : 0);
    LUA_FireEvent(event);
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
