#include "game/people.h"

#include "game/box.h"
#include "game/effects/gunshot.h"
#include "game/items.h"
#include "game/los.h"
#include "game/random.h"
#include "global/vars.h"

bool Targetable(ITEM_INFO *item, AI_INFO *info)
{
    if (!info->ahead || info->distance >= PEOPLE_SHOOT_RANGE) {
        return false;
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

    return LOS_Check(&start, &target);
}

bool ShotLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation)
{
    bool hit;
    if (distance > PEOPLE_SHOOT_RANGE) {
        hit = false;
    } else {
        hit = Random_GetControl()
            < ((PEOPLE_SHOOT_RANGE - distance) / (PEOPLE_SHOOT_RANGE / 0x7FFF)
               - PEOPLE_MISS_CHANCE);
    }

    int16_t fx_num;
    if (hit) {
        fx_num = CreatureEffect(item, gun, Effect_GunShotHit);
    } else {
        fx_num = CreatureEffect(item, gun, Effect_GunShotMiss);
    }

    if (fx_num != NO_ITEM) {
        g_Effects[fx_num].pos.y_rot += extra_rotation;
    }

    return hit;
}
