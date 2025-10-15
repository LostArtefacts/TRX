#include "game/cutscene.h"

#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/lara.h"
#include "game/shell.h"
#include "global/types.h"

#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/input.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara/hair.h>
#include <libtrx/game/level.h>
#include <libtrx/game/music.h>
#include <libtrx/game/output.h>
#include <libtrx/game/sound.h>
#include <libtrx/memory.h>

static void M_InitialiseLara(const GF_LEVEL *const level)
{
    const OBJECT_ID lara_type = level->lara_type;
    Lara_Hair_SetLaraType(lara_type);
    if (!Lara_Hair_IsActive()) {
        return;
    }

    if (lara_type == O_LARA) {
        return;
    }

    int16_t lara_item_num = NO_ITEM;
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        if (Item_Get(i)->object_id == lara_type) {
            lara_item_num = i;
            break;
        }
    }

    if (lara_item_num == NO_ITEM) {
        return;
    }

    Lara_InitialiseLoad(lara_item_num);
    Lara_Initialise(level);

    ITEM *const lara_item = Lara_GetItem();
    Item_SwitchToObjAnim(lara_item, 0, 0, lara_type);
    const ANIM *const cut_anim = Item_GetAnim(lara_item);
    lara_item->current_anim_state = lara_item->goal_anim_state =
        lara_item->required_anim_state = cut_anim->current_anim_state;
}

bool Cutscene_Start(const int32_t level_num)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, level_num);
    ASSERT(GF_GetCurrentLevel() == level);

    M_InitialiseLara(level);

    const int32_t room_count = Room_GetCount();
    for (int16_t room_num = 0; room_num < room_count; room_num++) {
        const ROOM *const room = Room_Get(room_num);
        if (room->flipped_room >= 0) {
            Room_Get(room->flipped_room)->bound_active = 1;
        }
    }

    Room_DrawReset();
    for (int16_t room_num = 0; room_num < room_count; room_num++) {
        if (!Room_Get(room_num)->bound_active) {
            Room_MarkToBeDrawn(room_num);
        }
    }

    Camera_GetCineData()->frame_idx = 0;

    if (level->music_track != MX_INACTIVE) {
        Music_Play_Direct(level->music_track, MPM_ALWAYS);
    }

    return true;
}

void Cutscene_End(void)
{
    Music_Stop();
}

GF_COMMAND Cutscene_Control(void)
{
    Interpolation_Remember();
    Music_SyncTimestamp(Camera_GetCineData()->frame_idx / (double)LOGIC_FPS);

    Input_Update();
    Shell_ProcessInput();
    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    } else if (g_InputDB.pause) {
        const GF_COMMAND gf_cmd = GF_PauseGame();
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    } else if (g_InputDB.toggle_photo_mode) {
        const GF_COMMAND gf_cmd = GF_EnterPhotoMode();
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }
    }

    Output_ResetDynamicLights();

    Item_Control();
    Effect_Control();
    Lara_Hair_Control(true);
    Camera_UpdateCutscene();
    Output_AnimateTextures(1);

    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count - 1) {
        // Remember the scene after the update to prevent the interpolation
        // from twitching the camera back and forth.
        Interpolation_Remember();

        cine_data->frame_idx = cine_data->frame_count - 2;
        return (GF_COMMAND) { .action = GF_LEVEL_COMPLETE };
    }

    return (GF_COMMAND) { .action = GF_NOOP };
}

void Cutscene_Draw(void)
{
    Game_Draw(true);
}
