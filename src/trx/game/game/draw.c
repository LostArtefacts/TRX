#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/cutseq.h>
#include <trx/game/interpolation.h>
#include <trx/game/output.h>
#include <trx/game/overlay.h>

void Game_Draw(const bool draw_overlay)
{
    Interpolation_Interpolate();
    CutSeq_PreDraw();
    Camera_Apply();
    Output_FlushPendingLights();
    Room_DrawAllRooms(g_Camera.interp.room_num, g_Camera.target.room_num);
    if (draw_overlay && !CutSeq_IsActive()) {
        Overlay_DrawGameInfo();
    }
    CutSeq_DrawOverlay();
    Output_Overlay_DrawLetterbox();
    SceneCompositor_Flush();
    if (g_Config.visuals.enable_reflections) {
        Output_Textures_UpdateEnvironmentMap();
    }
}
