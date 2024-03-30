#include "game/game.h"

#include "game/camera.h"
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
    Camera_Apply();

    if (g_Objects[O_LARA].loaded) {
        Room_DrawAllRooms(g_Camera.pos.room_number);
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

    Output_DrawBackdropScreen();
}
