#include "game/game.h"

#include "config.h"
#include "game/camera.h"
#include "game/interpolation.h"
#include "game/lara/lara_draw.h"
#include "game/lara/lara_hair.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/room_draw.h"
#include "game/viewport.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>

void Game_DrawScene(bool draw_overlay)
{
    Interpolation_Commit();
    Camera_Apply();

    if (g_Objects[O_LARA].loaded) {
        Room_DrawAllRooms(g_Camera.interp.room_num, g_Camera.target.room_num);

        if (g_Config.rendering.enable_reflections) {
            Output_FillEnvironmentMap();
        }

        if (g_RoomInfo[g_LaraItem->room_num].flags & RF_UNDERWATER) {
            Output_SetupBelowWater(g_Camera.underwater);
        } else {
            Output_SetupAboveWater(g_Camera.underwater);
        }

        Lara_Draw(g_LaraItem);

        if (draw_overlay) {
            Overlay_DrawGameInfo();
        } else {
            Overlay_HideGameInfo();
        }

    } else {
        // cinematic scene
        for (int i = 0; i < g_RoomsToDrawCount; i++) {
            int16_t room_num = g_RoomsToDraw[i];
            ROOM_INFO *r = &g_RoomInfo[room_num];
            r->top = 0;
            r->left = 0;
            r->right = Viewport_GetMaxX();
            r->bottom = Viewport_GetMaxY();
            Room_DrawSingleRoom(room_num);
        }

        Output_SetupAboveWater(false);
        Lara_Hair_Draw();
    }

    Output_FlushTranslucentObjects();
    Output_DrawBackdropScreen();
}
