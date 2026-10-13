#include <trx/game/lua/events/ui.h>

#include <trx/core/subsystem.h>
#include <trx/game/lua/events.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/fade.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>

static bool m_HideNextFrame = false;
// What the interface was laid out in when scripts were last told.
static float m_Layout[8] = {};

static void M_FadeDone(const int32_t id)
{
    LUA_FireEventInt32(LUA_EVENT_UI_FADE_DONE, id);
}

// Tells scripts when the room the interface lays out in has changed since
// the last tick: the canvas, the safe and working areas, or the text scale.
static void M_CheckLayout(void)
{
    const float layout[] = {
        UI_GetCanvasWidth(),         UI_GetCanvasHeight(),
        UI_GetSafeCanvasWidth(),     UI_GetSafeCanvasTop(),
        UI_GetSafeCanvasBottom(),    UI_GetWorkingCanvasTop(),
        UI_GetWorkingCanvasBottom(), UI_Scaler_GetScale(UI_SCALER_TARGET_TEXT),
    };
    bool changed = false;
    for (size_t i = 0; i < sizeof(layout) / sizeof(layout[0]); i++) {
        if (layout[i] != m_Layout[i]) {
            m_Layout[i] = layout[i];
            changed = true;
        }
    }
    if (changed) {
        LUA_FireEvent(LUA_EVENT_UI_LAYOUT);
    }
}

static void M_Tick(void)
{
    UI_Fade_Tick(M_FadeDone);
    M_CheckLayout();
}

static void M_Init(void)
{
    UI_SetPaintHook(LUA_UI_PaintRegions);
    LUA_SetTickHook(M_Tick);
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
