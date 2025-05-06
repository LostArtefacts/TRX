#include "game/spawn.h"

#include "game/effects.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/game/collision.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define SHARD_SPEED 250
#define ROCKET_SPEED 220

static void M_ShootAtLara(EFFECT *effect);

void M_ShootAtLara(EFFECT *effect)
{
    int32_t x = g_LaraItem->pos.x - effect->pos.x;
    int32_t y = g_LaraItem->pos.y - effect->pos.y;
    int32_t z = g_LaraItem->pos.z - effect->pos.z;

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(g_LaraItem);
    y += bounds->max.y + (bounds->min.y - bounds->max.y) * 3 / 4;

    int32_t dist = Math_Sqrt(SQUARE(x) + SQUARE(z));
    effect->rot.x = -(PHD_ANGLE)Math_Atan(dist, y);
    effect->rot.y = Math_Atan(z, x);
    effect->rot.x += (Random_GetControl() - 0x4000) / 0x40;
    effect->rot.y += (Random_GetControl() - 0x4000) / 0x40;
}

void Spawn_Bubble(const XYZ_32 *const pos, const int16_t room_num)
{
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = *pos;
    effect->object_id = O_BUBBLES_1;
    effect->frame_num = -((Random_GetDraw() * 3) / 0x8000);
    effect->speed = 10 + ((Random_GetDraw() * 6) / 0x8000);
}

void Spawn_Splash(ITEM *item)
{
    int16_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    Sound_Effect(SFX_LARA_SPLASH, &item->pos, SPM_NORMAL);

    for (int i = 0; i < 10; i++) {
        int16_t effect_num = Effect_Create(room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *effect = Effect_Get(effect_num);
            effect->pos.x = item->pos.x;
            effect->pos.y = wh;
            effect->pos.z = item->pos.z;
            effect->rot.y = DEG_180 + 2 * Random_GetDraw();
            effect->object_id = O_SPLASH_1;
            effect->frame_num = 0;
            effect->speed = Random_GetDraw() / 256;
        }
    }
}

int16_t Spawn_Blood(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num)
{
    int16_t effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        effect->pos.x = x;
        effect->pos.y = y;
        effect->pos.z = z;
        effect->rot.y = direction;
        effect->object_id = O_BLOOD_1;
        effect->frame_num = 0;
        effect->counter = 0;
        effect->speed = speed;
    }
    return effect_num;
}

int16_t Spawn_ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        effect->room_num = room_num;
        effect->pos.x = x;
        effect->pos.y = y;
        effect->pos.z = z;
        effect->rot.x = 0;
        effect->rot.y = y_rot;
        effect->rot.z = 0;
        effect->object_id = O_MISSILE_2;
        effect->frame_num = 0;
        effect->speed = SHARD_SPEED;
        effect->shade = 3584;
        M_ShootAtLara(effect);
    }
    return effect_num;
}

int16_t Spawn_RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num)
{
    int16_t effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        effect->room_num = room_num;
        effect->pos.x = x;
        effect->pos.y = y;
        effect->pos.z = z;
        effect->rot.x = 0;
        effect->rot.y = y_rot;
        effect->rot.z = 0;
        effect->object_id = O_MISSILE_3;
        effect->frame_num = 0;
        effect->speed = ROCKET_SPEED;
        effect->shade = SHADE_NEUTRAL;
        M_ShootAtLara(effect);
    }
    return effect_num;
}

int16_t Spawn_GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        effect->pos.x = x;
        effect->pos.y = y;
        effect->pos.z = z;
        effect->room_num = room_num;
        effect->rot.x = 0;
        effect->rot.y = y_rot;
        effect->rot.z = 0;
        effect->counter = 3;
        effect->frame_num = 0;
        effect->object_id = O_GUN_FLASH;
        effect->shade = SHADE_NEUTRAL;
    }
    return effect_num;
}

int16_t Spawn_GunShotHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    XYZ_32 pos = {
        .x = -((Random_GetDraw() - 0x4000) << 7) / 0x7FFF,
        .y = -((Random_GetDraw() - 0x4000) << 7) / 0x7FFF,
        .z = -((Random_GetDraw() - 0x4000) << 7) / 0x7FFF,
    };
    Collide_GetJointAbsPosition(
        g_LaraItem, &pos, (Random_GetControl() * LM_NUMBER_OF) / 0x7FFF);
    Spawn_Blood(
        pos.x, pos.y, pos.z, g_LaraItem->speed, g_LaraItem->rot.y,
        g_LaraItem->room_num);
    Sound_Effect(SFX_LARA_BULLETHIT, &g_LaraItem->pos, SPM_NORMAL);
    return Spawn_GunShot(x, y, z, speed, y_rot, room_num);
}

int16_t Spawn_GunShotMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    GAME_VECTOR pos;
    pos.x = g_LaraItem->pos.x
        + ((Random_GetDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.y = g_LaraItem->floor;
    pos.z = g_LaraItem->pos.z
        + ((Random_GetDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.room_num = g_LaraItem->room_num;
    Spawn_Ricochet(&pos);
    return Spawn_GunShot(x, y, z, speed, y_rot, room_num);
}

void Spawn_Ricochet(GAME_VECTOR *pos)
{
    int16_t effect_num = Effect_Create(pos->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        effect->pos.x = pos->x;
        effect->pos.y = pos->y;
        effect->pos.z = pos->z;
        effect->counter = 4;
        effect->object_id = O_RICOCHET_1;
        effect->frame_num = -3 * Random_GetDraw() / 0x8000;
        Sound_Effect(SFX_LARA_RICOCHET, &effect->pos, SPM_NORMAL);
    }
}

void Spawn_Twinkle(GAME_VECTOR *pos)
{
    int16_t effect_num = Effect_Create(pos->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *effect = Effect_Get(effect_num);
        effect->pos.x = pos->x;
        effect->pos.y = pos->y;
        effect->pos.z = pos->z;
        effect->counter = 0;
        effect->object_id = O_TWINKLE;
        effect->frame_num = 0;
    }
}
