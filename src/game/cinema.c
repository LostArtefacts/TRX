#include "game/cinema.h"

#include "3dsystem/3d_gen.h"
#include "game/camera.h"
#include "game/cinema.h"
#include "game/draw.h"
#include "game/input.h"
#include "game/items.h"
#include "game/level.h"
#include "game/music.h"
#include "game/output.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"

static const int32_t m_CinematicAnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

int32_t StartCinematic(int32_t level_num)
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

int32_t CinematicLoop(void)
{
    DoCinematic(2);
    Draw_ProcessFrame();
    int32_t nframes;
    do {
        nframes = Draw_ProcessFrame();
    } while (!DoCinematic(nframes));
    return GF_NOP;
}

int32_t StopCinematic(int32_t level_num)
{
    Music_Stop();
    Sound_StopAllSamples();

    g_LevelComplete = true;

    return level_num | GF_LEVEL_COMPLETE;
}

bool DoCinematic(int32_t nframes)
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

        int16_t item_num = g_NextItemActive;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &g_Items[item_num];
            OBJECT_INFO *object = &g_Objects[item->object_number];
            int16_t next_item_num = item->next_active;

            if (object->control) {
                object->control(item_num);
            }

            item_num = next_item_num;
        }

        int16_t fx_num = g_NextFxActive;
        while (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            OBJECT_INFO *object = &g_Objects[fx->object_number];
            int16_t next_fx_num = fx->next_active;

            if (object->control) {
                object->control(fx_num);
            }

            fx_num = next_fx_num;
        }

        Camera_UpdateCutscene();

        g_CineFrame++;
        m_FrameCount -= 0x10000;
    }

    return false;
}

void InitialisePlayer1(int16_t item_num)
{
    Item_AddActive(item_num);

    ITEM_INFO *item = &g_Items[item_num];
    g_Camera.pos.room_number = item->room_number;
    g_Camera.pos.x = item->pos.x;
    g_Camera.pos.y = item->pos.y;
    g_Camera.pos.z = item->pos.z;
    g_Camera.target_angle = 0;
    item->pos.y_rot = 0;
}

void ControlCinematicPlayer(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->pos.y_rot = g_Camera.target_angle;
    item->pos.x = g_Camera.pos.x;
    item->pos.y = g_Camera.pos.y;
    item->pos.z = g_Camera.pos.z;
    Item_Animate(item);
}

void ControlCinematicPlayer4(int16_t item_num)
{
    Item_Animate(&g_Items[item_num]);
}

void InitialiseGenPlayer(int16_t item_num)
{
    Item_AddActive(item_num);
    g_Items[item_num].pos.y_rot = 0;
}
