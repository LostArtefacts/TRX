#include "config.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/interpolation.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/rooms.h"

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
