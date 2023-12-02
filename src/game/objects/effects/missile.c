#include "game/objects/effects/missile.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "util.h"

#include <stdbool.h>

#define SHARD_DAMAGE 30
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE_BASE WALL_L
#define ROCKET_RANGE SQUARE(ROCKET_RANGE_BASE) // = 1048576

void Missile_Setup(OBJECT_INFO *obj)
{
    obj->control = Missile_Control;
}

void Missile_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];

    int32_t speed = (fx->speed * Math_Cos(fx->pos.x_rot)) >> W2V_SHIFT;
    fx->pos.y += (fx->speed * Math_Sin(-fx->pos.x_rot)) >> W2V_SHIFT;
    fx->pos.z += (speed * Math_Cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (speed * Math_Sin(fx->pos.y_rot)) >> W2V_SHIFT;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    int32_t height = Room_GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    int32_t ceiling = Room_GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z);

    if (fx->pos.y >= height || fx->pos.y <= ceiling) {
        if (fx->object_number == O_MISSILE2) {
            fx->object_number = O_RICOCHET1;
            fx->frame_number = -Random_GetControl() / 11000;
            fx->speed = 0;
            fx->counter = 6;
            Sound_Effect(SFX_LARA_RICOCHET, &fx->pos, SPM_NORMAL);
        } else {
            fx->object_number = O_EXPLOSION1;
            fx->frame_number = 0;
            fx->speed = 0;
            fx->counter = 0;
            Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);

            int32_t x = fx->pos.x - g_LaraItem->pos.x;
            int32_t y = fx->pos.y - g_LaraItem->pos.y;
            int32_t z = fx->pos.z - g_LaraItem->pos.z;
            if (Item_Test3DRange(x, y, z, ROCKET_RANGE_BASE)) {
                int32_t range = SQUARE(x) + SQUARE(y) + SQUARE(z);
                Lara_TakeDamage(
                    ROCKET_DAMAGE * (ROCKET_RANGE - range) / ROCKET_RANGE,
                    true);
            }
        }
        return;
    }

    if (room_num != fx->room_number) {
        Effect_NewRoom(fx_num, room_num);
    }

    if (!Lara_IsNearItem(&fx->pos, 200)) {
        return;
    }

    if (fx->object_number == O_MISSILE2) {
        Lara_TakeDamage(SHARD_DAMAGE, true);
        fx->object_number = O_BLOOD1;
        Sound_Effect(SFX_LARA_BULLETHIT, &fx->pos, SPM_NORMAL);
    } else {
        Lara_TakeDamage(ROCKET_DAMAGE, true);
        fx->object_number = O_EXPLOSION1;
        if (g_LaraItem->hit_points > 0) {
            Sound_Effect(SFX_LARA_INJURY, &g_LaraItem->pos, SPM_NORMAL);
            g_Lara.spaz_effect = fx;
            g_Lara.spaz_effect_count = 5;
        }
        Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
    }

    fx->frame_number = 0;
    fx->pos.y_rot = g_LaraItem->pos.y_rot;
    fx->speed = g_LaraItem->speed;
    fx->counter = 0;
}
