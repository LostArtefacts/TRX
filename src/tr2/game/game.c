#include "game/game.h"

#include "game/room_draw.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/output.h>
#include <libtrx/game/overlay.h>

void Game_Draw(const bool draw_overlay)
{
    Interpolation_Interpolate();
    Camera_Apply();
    Room_DrawAllRooms(g_Camera.interp.room_num);
    if (draw_overlay) {
        Overlay_DrawGameInfo();
    }
    SceneCompositor_Flush();
    if (g_Config.visuals.enable_reflections) {
        Output_Textures_UpdateEnvironmentMap();
    }
    Game_DrawFade();
}
