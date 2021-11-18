#include "game/cinema.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/music.h"
#include "game/setup.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_display.h"
#include "specific/s_input.h"

static bool SoundIsActiveOld = false;
static const int32_t CinematicAnimationRate = 0x8000;

int32_t StartCinematic(int32_t level_num)
{
    if (!InitialiseLevel(level_num, GFL_CUTSCENE)) {
        return END_ACTION;
    }

    InitCinematicRooms();

    SoundIsActiveOld = SoundIsActive;
    SoundIsActive = false;
    CineFrame = 0;
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
    SoundIsActive = SoundIsActiveOld;

    LevelComplete = true;
    S_FadeInInventory(1);

    return level_num | GF_LEVEL_COMPLETE;
}

void InitCinematicRooms()
{
    for (int16_t room_num = 0; room_num < RoomCount; room_num++) {
        if (RoomInfo[room_num].flipped_room >= 0) {
            RoomInfo[RoomInfo[room_num].flipped_room].bound_active = 1;
        }
    }

    RoomsToDrawCount = 0;
    for (int16_t room_num = 0; room_num < RoomCount; room_num++) {
        if (!RoomInfo[room_num].bound_active) {
            if (RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
                RoomsToDraw[RoomsToDrawCount++] = room_num;
            }
        }
    }
}

int32_t DoCinematic(int32_t nframes)
{
    static int32_t frame_count = 0;

    frame_count += CinematicAnimationRate * nframes;
    while (frame_count >= 0) {
        S_UpdateInput();
        if (g_Input.option) {
            return 1;
        }

        int16_t item_num = NextItemActive;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &Items[item_num];
            OBJECT_INFO *object = &Objects[item->object_number];
            int16_t next_item_num = item->next_active;

            if (object->control) {
                object->control(item_num);
            }

            item_num = next_item_num;
        }

        int16_t fx_num = NextFxActive;
        while (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            OBJECT_INFO *object = &Objects[fx->object_number];
            int16_t next_fx_num = fx->next_active;

            if (object->control) {
                object->control(fx_num);
            }

            fx_num = next_fx_num;
        }

        CalculateCinematicCamera();
        CineFrame++;

        if (CineFrame >= NumCineFrames) {
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

    int16_t *ptr = &Cine[8 * CineFrame];
    int32_t tx = ptr[0];
    int32_t ty = ptr[1];
    int32_t tz = ptr[2];
    int32_t cx = ptr[3];
    int32_t cy = ptr[4];
    int32_t cz = ptr[5];
    int16_t fov = ptr[6];
    int16_t roll = ptr[7];

    int32_t c = phd_cos(Camera.target_angle);
    int32_t s = phd_sin(Camera.target_angle);

    camtar.x = Camera.pos.x + ((tx * c + tz * s) >> W2V_SHIFT);
    camtar.y = Camera.pos.y + ty;
    camtar.z = Camera.pos.z + ((tz * c - tx * s) >> W2V_SHIFT);
    campos.x = Camera.pos.x + ((cz * s + cx * c) >> W2V_SHIFT);
    campos.y = Camera.pos.y + cy;
    campos.z = Camera.pos.z + ((cz * c - cx * s) >> W2V_SHIFT);

    phd_AlterFOV(fov);
    phd_LookAt(
        campos.x, campos.y, campos.z, camtar.x, camtar.y, camtar.z, roll);
}

void InitialisePlayer1(int16_t item_num)
{
    AddActiveItem(item_num);

    ITEM_INFO *item = &Items[item_num];
    Camera.pos.room_number = item->room_number;
    Camera.pos.x = item->pos.x;
    Camera.pos.y = item->pos.y;
    Camera.pos.z = item->pos.z;
    Camera.target_angle = 0;
    item->pos.y_rot = 0;
}

void ControlCinematicPlayer(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    item->pos.y_rot = Camera.target_angle;
    item->pos.x = Camera.pos.x;
    item->pos.y = Camera.pos.y;
    item->pos.z = Camera.pos.z;
    AnimateItem(item);
}

void ControlCinematicPlayer4(int16_t item_num)
{
    AnimateItem(&Items[item_num]);
}

void InitialiseGenPlayer(int16_t item_num)
{
    AddActiveItem(item_num);
    Items[item_num].pos.y_rot = 0;
}

void InGameCinematicCamera()
{
    CineFrame++;
    if (CineFrame >= NumCineFrames) {
        CineFrame = NumCineFrames - 1;
    }

    int16_t *ptr = &Cine[8 * CineFrame];
    int32_t tx = ptr[0];
    int32_t ty = ptr[1];
    int32_t tz = ptr[2];
    int32_t cx = ptr[3];
    int32_t cy = ptr[4];
    int32_t cz = ptr[5];
    int16_t fov = ptr[6];
    int16_t roll = ptr[7];

    int32_t c = phd_cos(CinePosition.y_rot);
    int32_t s = phd_sin(CinePosition.y_rot);

    Camera.target.x = CinePosition.x + ((c * tx + s * tz) >> 14);
    Camera.target.y = CinePosition.y + ty;
    Camera.target.z = CinePosition.z + ((c * tz - s * tx) >> 14);
    Camera.pos.x = CinePosition.x + ((s * cz + c * cx) >> 14);
    Camera.pos.y = CinePosition.y + cy;
    Camera.pos.z = CinePosition.z + ((c * cz - s * cx) >> 14);

    phd_AlterFOV(fov);

    phd_LookAt(
        Camera.pos.x, Camera.pos.y, Camera.pos.z, Camera.target.x,
        Camera.target.y, Camera.target.z, roll);
    GetFloor(Camera.pos.x, Camera.pos.y, Camera.pos.z, &Camera.pos.room_number);
}
