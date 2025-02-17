#include "game/objects/effects/missile.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define SHARD_DAMAGE 30
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE_BASE WALL_L
#define ROCKET_RANGE SQUARE(ROCKET_RANGE_BASE) // = 1048576

void Missile_Setup(OBJECT *obj)
{
    obj->control_func = Missile_Control;
}

void Missile_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);

    int32_t speed = (effect->speed * Math_Cos(effect->rot.x)) >> W2V_SHIFT;
    effect->pos.y += (effect->speed * Math_Sin(-effect->rot.x)) >> W2V_SHIFT;
    effect->pos.z += (speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.x += (speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector =
        Room_GetSector(effect->pos.x, effect->pos.y, effect->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, effect->pos.x, effect->pos.y, effect->pos.z);
    const int32_t ceiling =
        Room_GetCeiling(sector, effect->pos.x, effect->pos.y, effect->pos.z);

    if (effect->pos.y >= height || effect->pos.y <= ceiling) {
        if (effect->object_id == O_MISSILE_2) {
            effect->object_id = O_RICOCHET_1;
            effect->frame_num = -Random_GetControl() / 11000;
            effect->speed = 0;
            effect->counter = 6;
            Sound_Effect(SFX_LARA_RICOCHET, &effect->pos, SPM_NORMAL);
        } else {
            effect->object_id = O_EXPLOSION_1;
            effect->frame_num = 0;
            effect->speed = 0;
            effect->counter = 0;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &effect->pos, SPM_NORMAL);

            int32_t x = effect->pos.x - g_LaraItem->pos.x;
            int32_t y = effect->pos.y - g_LaraItem->pos.y;
            int32_t z = effect->pos.z - g_LaraItem->pos.z;
            if (Item_Test3DRange(x, y, z, ROCKET_RANGE_BASE)) {
                int32_t range = SQUARE(x) + SQUARE(y) + SQUARE(z);
                Lara_TakeDamage(
                    ROCKET_DAMAGE * (ROCKET_RANGE - range) / ROCKET_RANGE,
                    true);
            }
        }
        return;
    }

    if (room_num != effect->room_num) {
        Effect_NewRoom(effect_num, room_num);
    }

    if (!Lara_IsNearItem(&effect->pos, 200)) {
        return;
    }

    if (effect->object_id == O_MISSILE_2) {
        Lara_TakeDamage(SHARD_DAMAGE, true);
        effect->object_id = O_BLOOD_1;
        Sound_Effect(SFX_LARA_BULLETHIT, &effect->pos, SPM_NORMAL);
    } else {
        Lara_TakeDamage(ROCKET_DAMAGE, true);
        effect->object_id = O_EXPLOSION_1;
        if (g_LaraItem->hit_points > 0) {
            Sound_Effect(SFX_LARA_INJURY, &g_LaraItem->pos, SPM_NORMAL);
            g_Lara.hit_effect = effect;
            g_Lara.hit_effect_count = 5;
        }
        Sound_Effect(SFX_ATLANTEAN_EXPLODE, &effect->pos, SPM_NORMAL);
    }

    effect->frame_num = 0;
    effect->rot.y = g_LaraItem->rot.y;
    effect->speed = g_LaraItem->speed;
    effect->counter = 0;
}
