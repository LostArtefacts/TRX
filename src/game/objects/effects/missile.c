#include "game/objects/effects/missile.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <libtrx/utils.h>

#define SHARD_DAMAGE 30
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE_BASE WALL_L
#define ROCKET_RANGE SQUARE(ROCKET_RANGE_BASE) // = 1048576

void Missile_Setup(OBJECT *obj)
{
    obj->control = Missile_Control;
}

void Missile_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];

    int32_t speed = (fx->speed * Math_Cos(fx->rot.x)) >> W2V_SHIFT;
    fx->pos.y += (fx->speed * Math_Sin(-fx->rot.x)) >> W2V_SHIFT;
    fx->pos.z += (speed * Math_Cos(fx->rot.y)) >> W2V_SHIFT;
    fx->pos.x += (speed * Math_Sin(fx->rot.y)) >> W2V_SHIFT;

    int16_t room_num = fx->room_num;
    const SECTOR *const sector =
        Room_GetSector(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, fx->pos.x, fx->pos.y, fx->pos.z);
    const int32_t ceiling =
        Room_GetCeiling(sector, fx->pos.x, fx->pos.y, fx->pos.z);

    if (fx->pos.y >= height || fx->pos.y <= ceiling) {
        if (fx->object_id == O_MISSILE_2) {
            fx->object_id = O_RICOCHET_1;
            fx->frame_num = -Random_GetControl() / 11000;
            fx->speed = 0;
            fx->counter = 6;
            Sound_Effect(SFX_LARA_RICOCHET, &fx->pos, SPM_NORMAL);
        } else {
            fx->object_id = O_EXPLOSION_1;
            fx->frame_num = 0;
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

    if (room_num != fx->room_num) {
        Effect_NewRoom(fx_num, room_num);
    }

    if (!Lara_IsNearItem(&fx->pos, 200)) {
        return;
    }

    if (fx->object_id == O_MISSILE_2) {
        Lara_TakeDamage(SHARD_DAMAGE, true);
        fx->object_id = O_BLOOD_1;
        Sound_Effect(SFX_LARA_BULLETHIT, &fx->pos, SPM_NORMAL);
    } else {
        Lara_TakeDamage(ROCKET_DAMAGE, true);
        fx->object_id = O_EXPLOSION_1;
        if (g_LaraItem->hit_points > 0) {
            Sound_Effect(SFX_LARA_INJURY, &g_LaraItem->pos, SPM_NORMAL);
            g_Lara.spaz_effect = fx;
            g_Lara.spaz_effect_count = 5;
        }
        Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
    }

    fx->frame_num = 0;
    fx->rot.y = g_LaraItem->rot.y;
    fx->speed = g_LaraItem->speed;
    fx->counter = 0;
}
