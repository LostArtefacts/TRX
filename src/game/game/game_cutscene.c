#include "game/game.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_hair.h"
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
static void Game_Cutscene_InitialiseHair(int32_t level_num);

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

        Lara_Hair_Control(true);

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

    Game_Cutscene_InitialiseHair(level_num);

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

void Game_Cutscene_InitialiseHair(int32_t level_num)
{
    if (!g_Config.enable_braid || !g_Objects[O_HAIR].loaded) {
        return;
    }

    GAME_OBJECT_ID lara_type = g_GameFlow.levels[level_num].lara_type;
    if (lara_type == O_LARA) {
        return;
    }

    for (int i = 0; i < g_LevelItemCount; i++) {
        if (g_Items[i].object_number != lara_type) {
            continue;
        }

        Lara_InitialiseLoad(i);
        Lara_Initialise(level_num);
        Lara_Hair_SetLaraType(lara_type);

        g_LaraItem->anim_number = g_Objects[lara_type].anim_index;
        ANIM_STRUCT *cut_anim = &g_Anims[g_LaraItem->anim_number];
        g_LaraItem->frame_number = cut_anim->frame_base;
        g_LaraItem->current_anim_state = g_LaraItem->goal_anim_state =
            g_LaraItem->required_anim_state = cut_anim->current_anim_state;

        break;
    }
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
