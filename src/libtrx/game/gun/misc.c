#include "game/gun/misc.h"

#include "game/const.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/math.h"

void Gun_FindTargetPoint(const ITEM *const item, GAME_VECTOR *const target)
{
    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    const int32_t x = bounds->min.x + (bounds->max.x - bounds->min.x) / 2;
    const int32_t y = bounds->min.y + (bounds->max.y - bounds->min.y) / 3;
    const int32_t z = bounds->min.z + (bounds->max.z - bounds->min.z) / 2;
    const int32_t cy = Math_Cos(item->rot.y);
    const int32_t sy = Math_Sin(item->rot.y);
    target->pos.x = item->pos.x + ((cy * x + sy * z) >> W2V_SHIFT);
    target->pos.y = item->pos.y + y;
    target->pos.z = item->pos.z + ((cy * z - sy * x) >> W2V_SHIFT);
    target->room_num = item->room_num;
}

void Gun_AimWeapon(const WEAPON_INFO *const weapon, LARA_ARM *const arm)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const int16_t speed = weapon->aim_speed;

    int16_t dest_x = 0;
    int16_t dest_y = 0;
    if (arm->lock) {
        dest_y = lara->target_angles[0];
        dest_x = lara->target_angles[1];
    }

    if (arm->rot.y >= dest_y - speed && arm->rot.y <= dest_y + speed) {
        arm->rot.y = dest_y;
    } else if (arm->rot.y < dest_y) {
        arm->rot.y += speed;
    } else {
        arm->rot.y -= speed;
    }

    if (arm->rot.x >= dest_x - speed && arm->rot.x <= dest_x + speed) {
        arm->rot.x = dest_x;
    } else if (arm->rot.x < dest_x) {
        arm->rot.x += speed;
    } else {
        arm->rot.x -= speed;
    }

    arm->rot.z = 0;
}
