#include "game/cinema.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/input.h"
#include "game/items.h"
#include "game/music.h"
#include "game/setup.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_display.h"

static bool m_SoundIsActiveOld = false;
static const int32_t m_CinematicAnimationRate = 0x8000;

int32_t StartCinematic(int32_t level_num)
{
    if (!InitialiseLevel(level_num, GFL_CUTSCENE)) {
        return END_ACTION;
    }

    InitCinematicRooms();

    m_SoundIsActiveOld = g_SoundIsActive;
    g_SoundIsActive = false;
    g_CineFrame = 0;
    return GF_NOP;
}

int32_t CinematicLoop()
{
    DoCinematic(2);
    DrawPhaseCinematic();
    int32_t nframes;
    do {
        nframes = DrawPhaseCinematic();
    } while (!DoCinematic(nframes));
    return GF_NOP;
}

int32_t StopCinematic(int32_t level_num)
{
    Music_Stop();
    Sound_StopAllSamples();
    g_SoundIsActive = m_SoundIsActiveOld;

    g_LevelComplete = true;
    S_FadeInInventory(1);

    return level_num | GF_LEVEL_COMPLETE;
}

void InitCinematicRooms()
{
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
}

int32_t DoCinematic(int32_t nframes)
{
    static int32_t frame_count = 0;

    frame_count += m_CinematicAnimationRate * nframes;
    while (frame_count >= 0) {
        Input_Update();
        if (g_Input.option) {
            return 1;
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

        CalculateCinematicCamera();
        g_CineFrame++;

        if (g_CineFrame >= g_NumCineFrames) {
            return 1;
        }

        frame_count -= 0x10000;
    }

    return 0;
}

void CalculateCinematicCamera()
{
    PHD_VECTOR campos;
    PHD_VECTOR camtar;

    int16_t *ptr = &g_Cine[8 * g_CineFrame];
    int32_t tx = ptr[0];
    int32_t ty = ptr[1];
    int32_t tz = ptr[2];
    int32_t cx = ptr[3];
    int32_t cy = ptr[4];
    int32_t cz = ptr[5];
    int16_t fov = ptr[6];
    int16_t roll = ptr[7];

    int32_t c = phd_cos(g_Camera.target_angle);
    int32_t s = phd_sin(g_Camera.target_angle);

    camtar.x = g_Camera.pos.x + ((tx * c + tz * s) >> W2V_SHIFT);
    camtar.y = g_Camera.pos.y + ty;
    camtar.z = g_Camera.pos.z + ((tz * c - tx * s) >> W2V_SHIFT);
    campos.x = g_Camera.pos.x + ((cz * s + cx * c) >> W2V_SHIFT);
    campos.y = g_Camera.pos.y + cy;
    campos.z = g_Camera.pos.z + ((cz * c - cx * s) >> W2V_SHIFT);

    phd_AlterFOV(fov);
    phd_LookAt(
        campos.x, campos.y, campos.z, camtar.x, camtar.y, camtar.z, roll);
}

void InitialisePlayer1(int16_t item_num)
{
    AddActiveItem(item_num);

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
    AnimateItem(item);
}

void ControlCinematicPlayer4(int16_t item_num)
{
    AnimateItem(&g_Items[item_num]);
}

void InitialiseGenPlayer(int16_t item_num)
{
    AddActiveItem(item_num);
    g_Items[item_num].pos.y_rot = 0;
}

void InGameCinematicCamera()
{
    g_CineFrame++;
    if (g_CineFrame >= g_NumCineFrames) {
        g_CineFrame = g_NumCineFrames - 1;
    }

    int16_t *ptr = &g_Cine[8 * g_CineFrame];
    int32_t tx = ptr[0];
    int32_t ty = ptr[1];
    int32_t tz = ptr[2];
    int32_t cx = ptr[3];
    int32_t cy = ptr[4];
    int32_t cz = ptr[5];
    int16_t fov = ptr[6];
    int16_t roll = ptr[7];

    int32_t c = phd_cos(g_CinePosition.y_rot);
    int32_t s = phd_sin(g_CinePosition.y_rot);

    g_Camera.target.x = g_CinePosition.x + ((c * tx + s * tz) >> 14);
    g_Camera.target.y = g_CinePosition.y + ty;
    g_Camera.target.z = g_CinePosition.z + ((c * tz - s * tx) >> 14);
    g_Camera.pos.x = g_CinePosition.x + ((s * cz + c * cx) >> 14);
    g_Camera.pos.y = g_CinePosition.y + cy;
    g_Camera.pos.z = g_CinePosition.z + ((c * cz - s * cx) >> 14);

    phd_AlterFOV(fov);

    phd_LookAt(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z, g_Camera.target.x,
        g_Camera.target.y, g_Camera.target.z, roll);
    GetFloor(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
        &g_Camera.pos.room_number);
}
