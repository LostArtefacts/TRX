#include "game/people.h"

#include "game/creature.h"
#include "game/effects/gunshot.h"
#include "game/items.h"
#include "game/random.h"
#include "global/vars.h"

bool ShotLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation)
{
    bool hit;
    if (distance > CREATURE_SHOOT_RANGE) {
        hit = false;
    } else {
        hit = Random_GetControl()
            < ((CREATURE_SHOOT_RANGE - distance)
                   / (CREATURE_SHOOT_RANGE / 0x7FFF)
               - CREATURE_MISS_CHANCE);
    }

    int16_t fx_num;
    if (hit) {
        fx_num = Creature_Effect(item, gun, Effect_GunShotHit);
    } else {
        fx_num = Creature_Effect(item, gun, Effect_GunShotMiss);
    }

    if (fx_num != NO_ITEM) {
        g_Effects[fx_num].pos.y_rot += extra_rotation;
    }

    return hit;
}
