#include "game/game.h"

#include "game/camera.h"
#include "game/lara/draw.h"
#include "game/lara/hair.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/room_draw.h"
#include "game/viewport.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/interpolation.h>

#include <stdint.h>

void Game_Draw(bool draw_overlay)
{
    Interpolation_Commit();
    Camera_Apply();

    if (Object_Get(O_LARA)->loaded) {
        Room_DrawAllRooms(g_Camera.interp.room_num, g_Camera.target.room_num);

        if (g_Config.visuals.enable_reflections) {
            Output_FillEnvironmentMap();
        }

        if (Room_Get(g_LaraItem->room_num)->flags & RF_UNDERWATER) {
            Output_SetupBelowWater(g_Camera.underwater);
        } else {
            Output_SetupAboveWater(g_Camera.underwater);
        }
        Lara_Draw(g_LaraItem);
        Output_FlushTranslucentObjects();
        Output_SetupAboveWater(false);

        if (draw_overlay) {
            Overlay_DrawGameInfo();
        } else {
            Overlay_HideGameInfo();
        }

    } else {
        // cinematic scene
        for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
            const int16_t room_num = Room_DrawGetRoom(i);
            ROOM *const room = Room_Get(room_num);
            room->bound_top = 0;
            room->bound_left = 0;
            room->bound_right = Viewport_GetMaxX();
            room->bound_bottom = Viewport_GetMaxY();
            Room_DrawSingleRoom(room_num);
        }

        Output_SetupAboveWater(false);
        Lara_Hair_Draw();
        Output_FlushTranslucentObjects();
    }
}
