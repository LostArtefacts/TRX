#include "game/game.h"

#include "game/room_draw.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/output/scene_compositor.h>
#include <libtrx/game/output/state.h>
#include <libtrx/game/output/textures.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/viewport.h>

void Game_Draw(bool draw_overlay)
{
    Interpolation_Interpolate();
    Camera_Apply();

    if (Object_Get(O_LARA)->loaded) {
        Room_DrawAllRooms(g_Camera.interp.room_num, g_Camera.target.room_num);

        ITEM *const lara_item = Lara_GetItem();
        if (Room_Get(lara_item->room_num)->flags & RF_UNDERWATER) {
            Output_SetupBelowWater(g_Camera.underwater);
        } else {
            Output_SetupAboveWater(g_Camera.underwater);
        }
        Lara_Draw(lara_item);

        Output_SetupAboveWater(false);

        if (draw_overlay) {
            Overlay_DrawGameInfo();
        }
    } else {
        // cinematic scene
        for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
            const int16_t room_num = Room_DrawGetRoom(i);
            ROOM *const room = Room_Get(room_num);
            room->bound_top = Viewport_GetMinY(VIEWPORT_GAME);
            room->bound_left = Viewport_GetMinX(VIEWPORT_GAME);
            room->bound_right = Viewport_GetMaxX(VIEWPORT_GAME);
            room->bound_bottom = Viewport_GetMaxY(VIEWPORT_GAME);
            Room_DrawSingleRoom(room_num);
        }

        Output_SetupAboveWater(false);
        Lara_Hair_Draw();
    }

    SceneCompositor_Flush();
    if (g_Config.visuals.enable_reflections) {
        Output_Textures_UpdateEnvironmentMap();
    }

    Game_DrawFade();
}
