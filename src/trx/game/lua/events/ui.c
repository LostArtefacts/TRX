#include <trx/game/lua/events/ui.h>

#include <trx/core/subsystem.h>
#include <trx/game/lua/events.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>

#include <stddef.h>

static bool m_HideNextFrame = false;
static bool m_IsFrameHidden = false;
static bool m_HoldingScreen = false;
// What the interface was laid out in when scripts were last told.
static float m_Layout[9] = {};

// Tells scripts when the room the interface lays out in has changed since
// they were last told: the canvas, the safe and working areas, or the text
// and bar scales.
static void M_CheckLayout(void)
{
    const float layout[] = {
        UI_GetCanvasWidth(),
        UI_GetCanvasHeight(),
        UI_GetSafeCanvasWidth(),
        UI_GetSafeCanvasTop(),
        UI_GetSafeCanvasBottom(),
        UI_GetWorkingCanvasTop(),
        UI_GetWorkingCanvasBottom(),
        UI_Scaler_GetScale(UI_SCALER_TARGET_TEXT),
        UI_Scaler_GetScale(UI_SCALER_TARGET_BAR),
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
    M_CheckLayout();
}

static void M_Init(void)
{
    LUA_SetTickHook(M_Tick);
}

// Lets go of the screen with the scripts, so that a fade still holding it when
// the game switches mods does not go on holding it in the next one.
static void M_Shutdown(void)
{
    m_HoldingScreen = false;
    m_HideNextFrame = false;
    m_IsFrameHidden = false;
}

// Lets scripts draw one layer of the scene.
static void M_Paint(const UI_LAYER layer)
{
    LUA_EVENT_TYPE event;
    switch (layer) {
    case UI_LAYER_BACKDROP:
        event = LUA_EVENT_UI_PAINT_BACKDROP;
        break;
    case UI_LAYER_UNDER:
        event = LUA_EVENT_UI_PAINT;
        break;
    case UI_LAYER_FADE:
        event = LUA_EVENT_UI_PAINT_FADE;
        break;
    case UI_LAYER_EXIT:
        event = LUA_EVENT_UI_PAINT_EXIT;
        break;
    case UI_LAYER_CONSOLE:
        event = LUA_EVENT_UI_PAINT_CONSOLE;
        break;
    default:
        return;
    }
    // A hidden frame still fades: only the interface leaves the picture.
    if (m_IsFrameHidden && layer == UI_LAYER_UNDER) {
        return;
    }
    UI_BeginLayer(layer);
    LUA_UI_SetPainting(true);
    LUA_FireEvent(event);
    LUA_UI_SetPainting(false);
    UI_EndLayer(layer);
}

void LUA_UI_EndScene(void)
{
    UI_Region_Layout();
    // Reports a change of the safe area at once, so that scripts lay out
    // again in this frame rather than one frame after the regions changed.
    M_CheckLayout();
    M_Paint(UI_LAYER_BACKDROP);
    M_Paint(UI_LAYER_UNDER);
    M_Paint(UI_LAYER_FADE);
    M_Paint(UI_LAYER_CONSOLE);
    M_Paint(UI_LAYER_EXIT);
}

void LUA_UI_SetHoldingScreen(const bool holding)
{
    m_HoldingScreen = holding;
}

bool LUA_UI_IsHoldingScreen(void)
{
    return m_HoldingScreen;
}

void LUA_UI_HideNextFrame(void)
{
    m_HideNextFrame = true;
}

void LUA_UI_ReserveRegions(void)
{
    m_IsFrameHidden = m_HideNextFrame;
    m_HideNextFrame = false;
    if (m_IsFrameHidden) {
        return;
    }
    LUA_UI_SetReserving(true);
    for (int32_t i = 0; i < UI_REGION_NUMBER_OF; i++) {
        LUA_FireEventInt32(LUA_EVENT_UI_RESERVE, i);
    }
    LUA_UI_SetReserving(false);
}

REGISTER_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
