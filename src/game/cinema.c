#include "game/cinema.h"

#include "3dsystem/3d_gen.h"
#include "game/camera.h"
#include "game/draw.h"
#include "game/input.h"
#include "game/items.h"
#include "game/level.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"

static const int32_t m_CinematicAnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

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
