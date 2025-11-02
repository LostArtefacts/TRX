#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/interpolation.h>
#include <trx/game/output.h>
#include <trx/game/overlay.h>
#include <trx/game/ui.h>

static FADER m_Fader;

void Game_FadeToBlack(const int32_t duration)
{
    Fader_Init(
        &m_Fader, FADER_TRANSPARENT, FADER_BLACK, duration / (float)LOGIC_FPS);
}

void Game_DrawFade(void)
{
    UI_BeginFade(&m_Fader, true);
    UI_EndFade();
}

void Game_Draw(const bool draw_overlay)
{
    Interpolation_Interpolate();
    Camera_Apply();
    Room_DrawAllRooms(g_Camera.interp.room_num, g_Camera.target.room_num);
    if (draw_overlay) {
        Overlay_DrawGameInfo();
    }
    SceneCompositor_Flush();
    if (g_Config.visuals.enable_reflections) {
        Output_Textures_UpdateEnvironmentMap();
    }
    Game_DrawFade();
}
