#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/people.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "util.h"

#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224

int32_t Targetable(ITEM_INFO* item, AI_INFO* info)
{
    if (!info->ahead || info->distance >= PEOPLE_SHOOT_RANGE) {
        return 0;
    }

    GAME_VECTOR start;
    start.x = item->pos.x;
    start.y = item->pos.y - STEP_L * 3;
    start.z = item->pos.z;
    start.room_number = item->room_number;

    GAME_VECTOR target;
    target.x = LaraItem->pos.x;
    target.y = LaraItem->pos.y - STEP_L * 3;
    target.z = LaraItem->pos.z;

    return LOS(&start, &target);
}

void ControlGunShot(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
        return;
    }
    fx->pos.z_rot = GetRandomControl();
}

int16_t GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->room_number = room_num;
        fx->pos.x_rot = 0;
        fx->pos.y_rot = y_rot;
        fx->pos.z_rot = 0;
        fx->counter = 3;
        fx->frame_number = 0;
        fx->object_number = O_GUN_FLASH;
        fx->shade = 4096;
    }
    return fx_num;
}

int16_t GunHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    PHD_VECTOR pos;
    pos.x = 0;
    pos.y = 0;
    pos.z = 0;
    GetJointAbsPosition(LaraItem, &pos, (GetRandomControl() * 25) / 0x7FFF);
    DoBloodSplat(
        pos.x, pos.y, pos.z, LaraItem->speed, LaraItem->pos.y_rot,
        LaraItem->room_number);
    SoundEffect(50, &LaraItem->pos, 0);
    return GunShot(x, y, z, speed, y_rot, room_num);
}

void T1MInjectGamePeople()
{
    INJECT(0x00430D80, Targetable);
    INJECT(0x00430E00, ControlGunShot);
    INJECT(0x00430E40, GunShot);
    INJECT(0x00430EB0, GunHit);
}
