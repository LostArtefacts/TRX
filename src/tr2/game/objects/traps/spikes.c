#include "game/objects/traps/spikes.h"

#include "game/collide.h"
#include "game/items.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/vars.h"

#define SPIKE_DAMAGE 15

void Spikes_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (lara_item->hit_points < 0) {
        return;
    }
    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    int32_t num = Random_GetControl() / 24576;
    if (lara_item->gravity) {
        if (lara_item->fall_speed > GRAVITY) {
            lara_item->hit_points = -1;
            num = 20;
        }
    } else if (lara_item->speed < 30) {
        return;
    }

    lara_item->hit_points -= SPIKE_DAMAGE;
    for (int32_t i = 0; i < num; i++) {
        const XYZ_32 pos = {
            .x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256,
            .z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256,
            .y = lara_item->pos.y - Random_GetControl() / 64,
        };
        Spawn_Blood(
            pos.x, pos.y, pos.z, 20, Random_GetControl(), item->room_num);
    }

    if (lara_item->hit_points <= 0) {
        Item_SwitchToAnim(lara_item, LA_SPIKE_DEATH, 0);
        lara_item->current_anim_state = LS_DEATH;
        lara_item->goal_anim_state = LS_DEATH;
        lara_item->pos.y = item->pos.y;
        lara_item->gravity = 0;
    }
}

void Spikes_Setup(void)
{
    OBJECT *const obj = Object_Get(O_SPIKES);
    obj->collision_func = Spikes_Collision;
}
