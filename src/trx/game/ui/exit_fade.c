#include <trx/game/ui/exit_fade.h>

#include <trx/config.h>
#include <trx/game/fader.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>

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
    UI_BeginLayer(UI_LAYER_EXIT);
    UI_ScheduleDrawScreenBlack(Fader_GetCurrentValue(&m_Fader));
    UI_EndLayer(UI_LAYER_EXIT);
}
