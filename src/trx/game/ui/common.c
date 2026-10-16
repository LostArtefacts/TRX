#include <trx/game/ui/common.h>

#include <trx/config.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/output/overlay.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/events.h>
#include <trx/game/ui/exit_fade.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

static void (*m_PaintHook)(UI_LAYER);
static int32_t m_Layer = -1;

extern void UI_ClearDraw(void);

static void M_Init(void)
{
    UI_InitEvents();
    UI_InitText();
    UI_InitDraw();
}

static void M_Shutdown(void)
{
    UI_ShutdownDraw();
    UI_ShutdownText();
    UI_ShutdownEvents();
}

static void M_Paint(const UI_LAYER layer)
{
    if (m_PaintHook != nullptr) {
        UI_BeginLayer(layer);
        m_PaintHook(layer);
        UI_EndLayer(layer);
    }
}

void UI_BeginScene(void)
{
    UI_ClearDraw();
    UI_Region_Clear();
}

void UI_BeginLayer(const UI_LAYER layer)
{
    ASSERT(m_Layer == -1);
    m_Layer = layer;
}

void UI_EndLayer(const UI_LAYER layer)
{
    ASSERT(m_Layer == (int32_t)layer);
    m_Layer = -1;
}

UI_LAYER UI_GetLayer(void)
{
    ASSERT(m_Layer != -1);
    return (UI_LAYER)m_Layer;
}

void UI_SetPaintHook(void (*const hook)(UI_LAYER))
{
    m_PaintHook = hook;
}

void UI_EndScene(void)
{
    UI_Region_Layout();
    M_Paint(UI_LAYER_BACKDROP);
    M_Paint(UI_LAYER_UNDER);
    M_Paint(UI_LAYER_OVER);
    M_Paint(UI_LAYER_FADE);
    M_Paint(UI_LAYER_CONSOLE);
    UI_DrawExitFade();
}

int32_t UI_GetCanvasWidth(void)
{
    return UI_Scaler_CalcInverse(
        Viewport_GetWidth(VIEWPORT_UI), UI_SCALER_TARGET_GENERIC);
}

int32_t UI_GetCanvasHeight(void)
{
    return UI_Scaler_CalcInverse(
        Viewport_GetHeight(VIEWPORT_UI), UI_SCALER_TARGET_GENERIC);
}

float UI_GetWorkingCanvasTop(void)
{
    return UI_GetCanvasHeight() * Output_Overlay_GetLetterbox();
}

float UI_GetWorkingCanvasBottom(void)
{
    return UI_GetCanvasHeight() - UI_GetWorkingCanvasTop();
}

float UI_GetSafeCanvasWidth(void)
{
    return MAX(0.0f, UI_GetCanvasWidth() - 2.0f * UI_SCREEN_MARGIN);
}

float UI_GetSafeCanvasTop(void)
{
    return UI_GetSafeCanvasTopFor(-1.0f);
}

float UI_GetSafeCanvasBottom(void)
{
    return UI_GetSafeCanvasBottomFor(-1.0f);
}

float UI_GetSafeCanvasHeight(void)
{
    return UI_GetSafeCanvasHeightFor(-1.0f);
}

float UI_GetSafeCanvasTopFor(const float width)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    UI_Region_GetCenterBoxFor(width, &x, &y, &w, &h);
    return MAX(UI_SCREEN_MARGIN, y);
}

float UI_GetSafeCanvasBottomFor(const float width)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    UI_Region_GetCenterBoxFor(width, &x, &y, &w, &h);
    return MIN(UI_GetCanvasHeight() - UI_SCREEN_MARGIN, y + h);
}

float UI_GetSafeCanvasHeightFor(const float width)
{
    return MAX(
        0.0f, UI_GetSafeCanvasBottomFor(width) - UI_GetSafeCanvasTopFor(width));
}

float UI_ScaleX(const float x)
{
    return UI_Scaler_Calc(x * 0x10000, UI_SCALER_TARGET_GENERIC) / 0x10000.p0;
}

float UI_ScaleY(const float y)
{
    return UI_Scaler_Calc(y * 0x10000, UI_SCALER_TARGET_GENERIC) / 0x10000.p0;
}

REGISTER_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
