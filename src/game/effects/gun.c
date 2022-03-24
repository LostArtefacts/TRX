#include "game/effects/gun.h"

#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/random.h"
#include "global/vars.h"

#define SHARD_SPEED 250
#define ROCKET_SPEED 220

static void ShootAtLara(FX_INFO *fx);

void ShootAtLara(FX_INFO *fx)
{
    int32_t x = g_LaraItem->pos.x - fx->pos.x;
    int32_t y = g_LaraItem->pos.y - fx->pos.y;
    int32_t z = g_LaraItem->pos.z - fx->pos.z;

    int16_t *bounds = GetBoundsAccurate(g_LaraItem);
    y += bounds[FRAME_BOUND_MAX_Y]
        + (bounds[FRAME_BOUND_MIN_Y] - bounds[FRAME_BOUND_MAX_Y]) * 3 / 4;

    int32_t dist = phd_sqrt(SQUARE(x) + SQUARE(z));
    fx->pos.x_rot = -(PHD_ANGLE)phd_atan(dist, y);
    fx->pos.y_rot = phd_atan(z, x);
    fx->pos.x_rot += (Random_GetControl() - 0x4000) / 0x40;
    fx->pos.y_rot += (Random_GetControl() - 0x4000) / 0x40;
}

int16_t Effect_ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &g_Effects[fx_num];
        fx->room_number = room_num;
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->pos.x_rot = 0;
        fx->pos.y_rot = y_rot;
        fx->pos.z_rot = 0;
        fx->object_number = O_MISSILE2;
        fx->frame_number = 0;
        fx->speed = SHARD_SPEED;
        fx->shade = 3584;
        ShootAtLara(fx);
    }
    return fx_num;
}

int16_t Effect_RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &g_Effects[fx_num];
        fx->room_number = room_num;
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->pos.x_rot = 0;
        fx->pos.y_rot = y_rot;
        fx->pos.z_rot = 0;
        fx->object_number = O_MISSILE3;
        fx->frame_number = 0;
        fx->speed = ROCKET_SPEED;
        fx->shade = 4096;
        ShootAtLara(fx);
    }
    return fx_num;
}
