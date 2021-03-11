#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/cinema.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/setup.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/input.h"
#include "specific/sndpc.h"
#include "config.h"
#include "util.h"

int32_t StartCinematic(int32_t level_num)
{
    if (T1MConfig.disable_cine) { // T1M
        return level_num | GF_LEVELCOMPLETE;
    }

    CinematicLevel = level_num;
    if (!InitialiseLevel(level_num)) {
        return END_ACTION;
    }

    InitCinematicRooms();

    switch (level_num) {
    case LV_CUTSCENE1:
        Camera.pos.x = 36668;
        Camera.pos.z = 63180;
        Camera.target_angle = -23312;
        S_StartSyncedAudio(23);
        break;

    case LV_CUTSCENE2:
        Camera.pos.x = 51962;
        Camera.pos.z = 53760;
        Camera.target_angle = 16380;
        S_StartSyncedAudio(25);
        break;

    case LV_CUTSCENE3:
        Camera.target_angle = PHD_90;
        FlipMap();
        S_StartSyncedAudio(24);
        break;

    case LV_CUTSCENE4:
        Camera.target_angle = PHD_90;
        S_StartSyncedAudio(22);
        break;
    }

    int32_t old_sound_is_active = SoundIsActive;
    SoundIsActive = 0;
    CineFrame = 0;

    DoCinematic(2);
    DrawPhaseCinematic();
    int32_t nframes;
    do {
        nframes = DrawPhaseCinematic();
    } while (!DoCinematic(nframes));

    S_CDStop();
    S_SoundStopAllSamples();
    SoundIsActive = old_sound_is_active;

    LevelComplete = 1;
    S_FadeInInventory(1);

    return level_num | GF_LEVELCOMPLETE;
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
    CinematicFrameCount += CinematicAnimationRate * nframes;
    while (CinematicFrameCount >= 0) {
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

        if (CineFrame >= NumCineFrames) {
            return 1;
        }

        CinematicFrameCount -= 0x10000;
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

    if (CinematicLevel == LV_CUTSCENE2 || CinematicLevel == LV_CUTSCENE4) {
        int16_t *temp;

        temp = Meshes[Objects[O_PLAYER_1].mesh_index + LM_THIGH_L];
        Meshes[Objects[O_PLAYER_1].mesh_index + LM_THIGH_L] =
            Meshes[Objects[O_PISTOLS].mesh_index + LM_THIGH_L];
        Meshes[Objects[O_PISTOLS].mesh_index + LM_THIGH_L] = temp;

        temp = Meshes[Objects[O_PLAYER_1].mesh_index + LM_THIGH_R];
        Meshes[Objects[O_PLAYER_1].mesh_index + LM_THIGH_R] =
            Meshes[Objects[O_PISTOLS].mesh_index + LM_THIGH_R];
        Meshes[Objects[O_PISTOLS].mesh_index + LM_THIGH_R] = temp;
    }
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

    int32_t c = phd_cos(CinematicPosition.y_rot);
    int32_t s = phd_sin(CinematicPosition.y_rot);

    Camera.target.x = CinematicPosition.x + ((c * tx + s * tz) >> 14);
    Camera.target.y = CinematicPosition.y + ty;
    Camera.target.z = CinematicPosition.z + ((c * tz - s * tx) >> 14);
    Camera.pos.x = CinematicPosition.x + ((s * cz + c * cx) >> 14);
    Camera.pos.y = CinematicPosition.y + cy;
    Camera.pos.z = CinematicPosition.z + ((c * cz - s * cx) >> 14);

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
