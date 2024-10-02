#include "game/effects/gun.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <libtrx/utils.h>

#define SHARD_SPEED 250
#define ROCKET_SPEED 220

static void M_ShootAtLara(FX *fx);

void M_ShootAtLara(FX *fx)
{
    int32_t x = g_LaraItem->pos.x - fx->pos.x;
    int32_t y = g_LaraItem->pos.y - fx->pos.y;
    int32_t z = g_LaraItem->pos.z - fx->pos.z;

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(g_LaraItem);
    y += bounds->max.y + (bounds->min.y - bounds->max.y) * 3 / 4;

    int32_t dist = Math_Sqrt(SQUARE(x) + SQUARE(z));
    fx->rot.x = -(PHD_ANGLE)Math_Atan(dist, y);
    fx->rot.y = Math_Atan(z, x);
    fx->rot.x += (Random_GetControl() - 0x4000) / 0x40;
    fx->rot.y += (Random_GetControl() - 0x4000) / 0x40;
}

int16_t Effect_ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = Effect_Create(room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->room_num = room_num;
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->rot.x = 0;
        fx->rot.y = y_rot;
        fx->rot.z = 0;
        fx->object_id = O_MISSILE_2;
        fx->frame_num = 0;
        fx->speed = SHARD_SPEED;
        fx->shade = 3584;
        M_ShootAtLara(fx);
    }
    return fx_num;
}

int16_t Effect_RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num)
{
    int16_t fx_num = Effect_Create(room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->room_num = room_num;
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->rot.x = 0;
        fx->rot.y = y_rot;
        fx->rot.z = 0;
        fx->object_id = O_MISSILE_3;
        fx->frame_num = 0;
        fx->speed = ROCKET_SPEED;
        fx->shade = 4096;
        M_ShootAtLara(fx);
    }
    return fx_num;
}
