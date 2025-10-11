#include "game/spawn.h"

#include "game/effects.h"
#include "game/objects/effects/missile_common.h"

#include <libtrx/game/math.h>
#include <libtrx/game/output.h>
#include <libtrx/game/random.h>

#define BARTOLI_LIGHT_RANGE (5 * WALL_L) // = 5120

int16_t Spawn_FireStream(
    const int32_t x, const int32_t y, const int32_t z, int16_t speed,
    const int16_t y_rot, const int16_t room_num)
{
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_EFFECT) {
        return effect_num;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos.x = x;
    effect->pos.y = y;
    effect->pos.z = z;
    effect->rot.x = 0;
    effect->rot.y = y_rot;
    effect->rot.z = 0;
    effect->room_num = room_num;
    effect->speed = 200;
    effect->frame_num =
        ((Object_Get(O_MISSILE_FLAME)->mesh_count + 1) * Random_GetDraw())
        >> 15;
    effect->object_id = O_MISSILE_FLAME;
    effect->shade = 14 * 256;

    Missile_ShootAtLara(effect);

    if (Object_Get(O_DRAGON_FRONT)->loaded) {
        effect->counter = 0x4000;
    } else {
        effect->counter = 20;
    }

    return effect_num;
}

void Spawn_MysticLight(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);

    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_TWINKLE;

        effect->rot.y = 2 * Random_GetDraw();
        effect->pos.x = item->pos.x
            + ((BARTOLI_LIGHT_RANGE * Math_Sin(effect->rot.y)) >> W2V_SHIFT);
        effect->pos.z = item->pos.z
            + ((BARTOLI_LIGHT_RANGE * Math_Cos(effect->rot.y)) >> W2V_SHIFT);
        effect->pos.y = (Random_GetDraw() >> 2) + item->pos.y - WALL_L;
        effect->room_num = item->room_num;
        effect->counter = item_num;
        effect->frame_num = 0;
    }

    // clang-format off
    Output_AddDynamicLight(
        item->pos,
        ((4 * Random_GetDraw()) >> 15) + 12,
        ((4 * Random_GetDraw()) >> 15) + 10);
    // clang-format on
}

int16_t Spawn_Knife(
    const int32_t x, const int32_t y, const int32_t z, const int16_t speed,
    const int16_t y_rot, const int16_t room_num)
{
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_EFFECT) {
        return effect_num;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos.x = x;
    effect->pos.y = y;
    effect->pos.z = z;
    effect->room_num = room_num;
    effect->rot.x = 0;
    effect->rot.y = y_rot;
    effect->rot.z = 0;
    effect->speed = 150;
    effect->frame_num = 0;
    effect->object_id = O_MISSILE_KNIFE;
    effect->shade = 3584;
    Missile_ShootAtLara(effect);
    return effect_num;
}
