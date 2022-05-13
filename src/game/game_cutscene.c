#include "game/game.h"

#include "game/cinema.h"
#include "game/draw.h"
#include "game/level.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdint.h>

int32_t Game_Cutscene_Start(int32_t level_num)
{
    if (!Level_Initialise(level_num)) {
        return END_ACTION;
    }

    for (int16_t room_num = 0; room_num < g_RoomCount; room_num++) {
        if (g_RoomInfo[room_num].flipped_room >= 0) {
            g_RoomInfo[g_RoomInfo[room_num].flipped_room].bound_active = 1;
        }
    }

    g_RoomsToDrawCount = 0;
    for (int16_t room_num = 0; room_num < g_RoomCount; room_num++) {
        if (!g_RoomInfo[room_num].bound_active) {
            if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
                g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
            }
        }
    }

    g_CineFrame = 0;
    return GF_NOP;
}

int32_t Game_Cutscene_Loop(void)
{
    DoCinematic(2);
    Draw_ProcessFrame();
    int32_t nframes;
    do {
        nframes = Draw_ProcessFrame();
    } while (!DoCinematic(nframes));
    return GF_NOP;
}
