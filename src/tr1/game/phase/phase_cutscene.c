#include "game/phase/phase_cutscene.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/level.h"
#include "game/music.h"
#include "game/output.h"
#include "game/phase/phase_pause.h"
#include "game/phase/phase_photo_mode.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/memory.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool m_PauseCutscene = false;

static void M_InitialiseHair(int32_t level_num);

static void M_Start(const PHASE_CUTSCENE_ARGS *args);
static void M_End(void);
static PHASE_CONTROL M_Control(int32_t nframes);
static void M_Draw(void);

static void M_InitialiseHair(int32_t level_num)
{
    const GAME_OBJECT_ID lara_type = g_GameFlow.levels[level_num].lara_type;
    Lara_Hair_SetLaraType(lara_type);
    if (!Lara_Hair_IsActive()) {
        return;
    }

    if (lara_type == O_LARA) {
        return;
    }

    int16_t lara_item_num = NO_ITEM;
    for (int i = 0; i < g_LevelItemCount; i++) {
        if (g_Items[i].object_id == lara_type) {
            lara_item_num = i;
            break;
        }
    }

    if (lara_item_num == NO_ITEM) {
        return;
    }

    Lara_InitialiseLoad(lara_item_num);
    Lara_Initialise(level_num);

    Item_SwitchToObjAnim(g_LaraItem, 0, 0, lara_type);
    ANIM *cut_anim = &g_Anims[g_LaraItem->anim_num];
    g_LaraItem->current_anim_state = g_LaraItem->goal_anim_state =
        g_LaraItem->required_anim_state = cut_anim->current_anim_state;
}

static void M_Start(const PHASE_CUTSCENE_ARGS *const args)
{
    m_PauseCutscene = false;

    // The cutscene is already playing and it's to be resumed.
    if (args->resume_existing) {
        Music_SeekTimestamp(g_CineFrame / (double)LOGIC_FPS);
        return;
    }

    Output_FadeReset();

    if (!Level_Initialise(args->level_num)) {
        return;
    }
    g_GameInfo.current_level_type = GFL_CUTSCENE;

    M_InitialiseHair(args->level_num);

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
}

static void M_End(void)
{
    if (m_PauseCutscene) {
        return;
    }

    Music_Stop();
    Sound_StopAllSamples();
}

static PHASE_CONTROL M_Control(int32_t nframes)
{
    Interpolation_Remember();

    for (int i = 0; i < nframes; i++) {
        if (g_CineFrame >= g_NumCineFrames - 1) {
            g_LevelComplete = true;
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_LEVEL_COMPLETE,
                    .param = g_CurrentLevel,
                },
            };
        }

        Input_Update();
        Shell_ProcessInput();
        Game_ProcessInput();

        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            g_LevelComplete = true;
            return (PHASE_CONTROL) {
                .end = true,
                .command = {
                    .action = GF_LEVEL_COMPLETE,
                    .param = g_CurrentLevel,
                },
            };
        }

        Item_Control();
        Effect_Control();
        Lara_Hair_Control();
        Camera_UpdateCutscene();

        g_CineFrame++;

        if (g_InputDB.toggle_photo_mode || g_InputDB.pause) {
            PHASE_CUTSCENE_ARGS *const cutscene_args =
                Memory_Alloc(sizeof(PHASE_CUTSCENE_ARGS));
            cutscene_args->resume_existing = true;
            cutscene_args->level_num = g_CurrentLevel;

            if (g_InputDB.toggle_photo_mode) {
                PHASE_PHOTO_MODE_ARGS *const args =
                    Memory_Alloc(sizeof(PHASE_PHOTO_MODE_ARGS));
                args->phase_to_return_to = PHASE_CUTSCENE;
                args->phase_arg = cutscene_args;
                Phase_Set(PHASE_PHOTO_MODE, args);
            } else if (g_InputDB.pause) {
                PHASE_PAUSE_ARGS *const args =
                    Memory_Alloc(sizeof(PHASE_PAUSE_ARGS));
                args->phase_to_return_to = PHASE_CUTSCENE;
                args->phase_arg = cutscene_args;
                Phase_Set(PHASE_PAUSE, args);
            }

            m_PauseCutscene = true;
            return (PHASE_CONTROL) { .end = false };
        }
    }

    return (PHASE_CONTROL) { .end = false };
}

static void M_Draw(void)
{
    Game_DrawScene(true);
    Output_AnimateTextures();
    Output_AnimateFades();
}

PHASER g_CutscenePhaser = {
    .start = (PHASER_START)M_Start,
    .end = M_End,
    .control = M_Control,
    .draw = M_Draw,
    .wait = NULL,
};
