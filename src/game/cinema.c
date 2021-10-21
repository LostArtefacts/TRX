#include "game/cinema.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/setup.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/display.h"
#include "specific/input.h"
#include "specific/sndpc.h"
#include "util.h"

static int32_t OldSoundIsActive;
static const int32_t CinematicAnimationRate = 0x8000;

//extern PHD_3DPOS_F LaraFloatPos;

int32_t StartCinematic(int32_t level_num)
{
    if (!InitialiseLevel(level_num, GFL_CUTSCENE)) {
        return END_ACTION;
    }

    InitCinematicRooms();

    OldSoundIsActive = SoundIsActive;
    SoundIsActive = 0;
    CineFrame = 0;
    return GF_NOP;
}

int32_t CinematicLoop()
{
    DoCinematic(2 / AnimScale);
    DrawPhaseCinematic();
    int32_t nframes;
    do {
        nframes = DrawPhaseCinematic();
    } while (!DoCinematic(nframes));
    return GF_NOP;
}

int32_t StopCinematic(int32_t level_num)
{
    S_MusicStop();
    S_SoundStopAllSamples();
    SoundIsActive = OldSoundIsActive;

    UpdateItemFloatPosFromFixed(LaraItem);

    LevelComplete = 1;
    S_FadeInInventory(1);

    return level_num | GF_LEVEL_COMPLETE;
}

void InitCinematicRooms()
{
    for (int i = 0; i < RoomCount; i++) {
        if (RoomInfo[i].flipped_room >= 0) {
            RoomInfo[RoomInfo[i].flipped_room].bound_active = 1;
        }
    }

    RoomsToDrawNum = 0;
    for (int i = 0; i < RoomCount; i++) {
        if (!RoomInfo[i].bound_active) {
            RoomsToDraw[RoomsToDrawNum++] = i;
        }
    }
}

int32_t DoCinematic(int32_t nframes)
{
    static int32_t frame_count = 0;

    frame_count += CinematicAnimationRate * nframes;
    while (frame_count >= 0) {
        S_UpdateInput();
        if (Input & IN_OPTION) {
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

        if (CineFrame / AnimScale >= NumCineFrames - 1) {
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

    int16_t *ptr = &Cine[8 * (CineFrame / AnimScale)];
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

    AlterFOV(fov);
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
    int16_t last_frame_num = NumCineFrames - (1 * AnimScale);
    if (CineFrame / AnimScale >= NumCineFrames) {
        CineFrame = last_frame_num;
    }

    // brackets are needed, it crashes otherwise
    int16_t *ptr = &Cine[8 * (CineFrame / AnimScale)];

    int32_t tx = ptr[0];
    int32_t ty = ptr[1];
    int32_t tz = ptr[2];
    int32_t cx = ptr[3];
    int32_t cy = ptr[4];
    int32_t cz = ptr[5];
    int16_t fov = ptr[6];
    int16_t roll = ptr[7];

    if (AnimScale == 2) {
        if (CineFrame & 1 && CineFrame < last_frame_num) {
            ptr += 8; // move to next frame
            tx += ptr[0];
            ty += ptr[1];
            tz += ptr[2];
            cx += ptr[3];
            cy += ptr[4];
            cz += ptr[5];
            fov += ptr[6];
            roll += ptr[7];

            tx /= AnimScale;
            ty /= AnimScale;
            tz /= AnimScale;
            cx /= AnimScale;
            cy /= AnimScale;
            cz /= AnimScale;
            fov /= AnimScale;
            roll /= AnimScale;
        }
    }

    int32_t c = phd_cos(CinePosition.y_rot);
    int32_t s = phd_sin(CinePosition.y_rot);

    Camera.target.x = CinePosition.x + ((c * tx + s * tz) >> 14);
    Camera.target.y = CinePosition.y + ty;
    Camera.target.z = CinePosition.z + ((c * tz - s * tx) >> 14);
    Camera.pos.x = CinePosition.x + ((s * cz + c * cx) >> 14);
    Camera.pos.y = CinePosition.y + cy;
    Camera.pos.z = CinePosition.z + ((c * cz - s * cx) >> 14);

    AlterFOV(fov);

    phd_LookAt(
        Camera.pos.x, Camera.pos.y, Camera.pos.z, Camera.target.x,
        Camera.target.y, Camera.target.z, roll);
    GetFloor(Camera.pos.x, Camera.pos.y, Camera.pos.z, &Camera.pos.room_number);
}

void T1MInjectGameCinema()
{
    INJECT(0x004110A0, StartCinematic);
    INJECT(0x00411240, DoCinematic);
    INJECT(0x00411370, CalculateCinematicCamera);
    INJECT(0x004114A0, ControlCinematicPlayer);
    INJECT(0x004114F0, InitialisePlayer1);
    INJECT(0x004115C0, InitialiseGenPlayer);
    INJECT(0x004115F0, InGameCinematicCamera);
}
