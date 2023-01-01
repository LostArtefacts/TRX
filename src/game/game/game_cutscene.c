#include "game/game.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/input.h"
#include "game/items.h"
#include "game/level.h"
#include "game/music.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>

static const int32_t m_CinematicAnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

static bool Game_Cutscene_Control(int32_t nframes);

bool Game_Cutscene_Control(int32_t nframes)
{
    m_FrameCount += m_CinematicAnimationRate * nframes;
    while (m_FrameCount >= 0) {
        if (g_CineFrame >= g_NumCineFrames - 1) {
            return true;
        }

        Input_Update();
        if (g_InputDB.deselect || g_InputDB.select) {
            return true;
        }

        Item_Control();
        Effect_Control();

        Camera_UpdateCutscene();

        g_CineFrame++;
        m_FrameCount -= 0x10000;
    }

    return false;
}

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

int32_t Game_Cutscene_Stop(int32_t level_num)
{
    Music_Stop();
    Sound_StopAllSamples();

    g_LevelComplete = true;

    return level_num | GF_LEVEL_COMPLETE;
}

int32_t Game_Cutscene_Loop(void)
{
    Game_Cutscene_Control(2);
    Game_ProcessFrame();
    int32_t nframes;
    do {
        nframes = Game_ProcessFrame();
    } while (!Game_Cutscene_Control(nframes));
    return GF_NOP;
}
