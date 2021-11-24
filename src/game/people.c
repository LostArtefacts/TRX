#include "game/people.h"

#include "game/box.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/effects/ricochet.h"
#include "game/items.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/sphere.h"
#include "global/vars.h"

int32_t Targetable(ITEM_INFO *item, AI_INFO *info)
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
    target.x = g_LaraItem->pos.x;
    target.y = g_LaraItem->pos.y - STEP_L * 3;
    target.z = g_LaraItem->pos.z;

    return LOS(&start, &target);
}

void ControlGunShot(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
        return;
    }
    fx->pos.z_rot = Random_GetControl();
}

int16_t GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &g_Effects[fx_num];
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
    GetJointAbsPosition(g_LaraItem, &pos, (Random_GetControl() * 25) / 0x7FFF);
    DoBloodSplat(
        pos.x, pos.y, pos.z, g_LaraItem->speed, g_LaraItem->pos.y_rot,
        g_LaraItem->room_number);
    Sound_Effect(SFX_LARA_BULLETHIT, &g_LaraItem->pos, SPM_NORMAL);
    return GunShot(x, y, z, speed, y_rot, room_num);
}

int16_t GunMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    GAME_VECTOR pos;
    pos.x = g_LaraItem->pos.x
        + ((Random_GetDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.y = g_LaraItem->floor;
    pos.z = g_LaraItem->pos.z
        + ((Random_GetDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.room_number = g_LaraItem->room_number;
    Ricochet(&pos);
    return GunShot(x, y, z, speed, y_rot, room_num);
}

int32_t ShotLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation)
{
    int32_t hit;
    if (distance > PEOPLE_SHOOT_RANGE) {
        hit = 0;
    } else {
        hit = Random_GetControl()
            < ((PEOPLE_SHOOT_RANGE - distance) / (PEOPLE_SHOOT_RANGE / 0x7FFF)
               - PEOPLE_MISS_CHANCE);
    }

    int16_t fx_num;
    if (hit) {
        fx_num = CreatureEffect(item, gun, GunHit);
    } else {
        fx_num = CreatureEffect(item, gun, GunMiss);
    }

    if (fx_num != NO_ITEM) {
        g_Effects[fx_num].pos.y_rot += extra_rotation;
    }

    return hit;
}
