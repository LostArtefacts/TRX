#include "game/effects/missile.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupMissile(OBJECT_INFO *obj)
{
    obj->control = ControlMissile;
}

void ControlMissile(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];

    int32_t speed = (fx->speed * phd_cos(fx->pos.x_rot)) >> W2V_SHIFT;
    fx->pos.y += (fx->speed * phd_sin(-fx->pos.x_rot)) >> W2V_SHIFT;
    fx->pos.z += (speed * phd_cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (speed * phd_sin(fx->pos.y_rot)) >> W2V_SHIFT;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    int32_t height = GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    int32_t ceiling = GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z);

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
            int32_t range = SQUARE(x) + SQUARE(y) + SQUARE(z);
            if (range < ROCKET_RANGE) {
                g_LaraItem->hit_points -=
                    (int16_t)(ROCKET_DAMAGE * (ROCKET_RANGE - range) / ROCKET_RANGE);
                g_LaraItem->hit_status = 1;
            }
        }
        return;
    }

    if (room_num != fx->room_number) {
        EffectNewRoom(fx_num, room_num);
    }

    if (!ItemNearLara(&fx->pos, 200)) {
        return;
    }

    if (fx->object_number == O_MISSILE2) {
        g_LaraItem->hit_points -= SHARD_DAMAGE;
        fx->object_number = O_BLOOD1;
        Sound_Effect(SFX_LARA_BULLETHIT, &fx->pos, SPM_NORMAL);
    } else {
        g_LaraItem->hit_points -= ROCKET_DAMAGE;
        fx->object_number = O_EXPLOSION1;
        if (g_LaraItem->hit_points > 0) {
            Sound_Effect(SFX_LARA_INJURY, &g_LaraItem->pos, SPM_NORMAL);
            g_Lara.spaz_effect = fx;
            g_Lara.spaz_effect_count = 5;
        }
        Sound_Effect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
    }
    g_LaraItem->hit_status = 1;

    fx->frame_number = 0;
    fx->pos.y_rot = g_LaraItem->pos.y_rot;
    fx->speed = g_LaraItem->speed;
    fx->counter = 0;
}

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

int16_t ShardGun(
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

int16_t RocketGun(
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
