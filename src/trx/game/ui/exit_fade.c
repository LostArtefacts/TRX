#include <trx/game/ui/exit_fade.h>

#include <trx/config.h>
#include <trx/game/fader.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/viewport.h>

static bool m_Started = false;
static FADER m_Fader = {};

void UI_BeginExitFade(void)
{
    if (m_Started) {
        return;
    }
    m_Started = true;
    if (g_Config.visuals.enable_exit_fade_effects) {
        Fader_InitFromCurrentHold(&m_Fader, 1.0f, 0.333f, 0.1f);
    }
}

bool UI_IsExitFading(void)
{
    return Fader_IsActive(&m_Fader);
}

void UI_DrawExitFade(void)
{
    const float opacity = Fader_GetCurrentValue(&m_Fader);
    if (opacity <= 0.0f) {
        return;
    }
    UI_BeginLayer(UI_LAYER_EXIT);
    UI_ScheduleDrawScreenFlatQuad(
        0, 0, 0, Viewport_GetWidth(VIEWPORT_UI),
        Viewport_GetHeight(VIEWPORT_UI),
        (RGBA_8888) { .a = (uint8_t)(opacity * 255.0f + 0.5f) });
    UI_EndLayer(UI_LAYER_EXIT);
}
