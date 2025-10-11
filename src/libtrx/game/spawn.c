#include "game/spawn.h"

#include "game/collision.h"
#include "game/effects.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"

void Spawn_Splash(const ITEM *const item)
{
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    for (int32_t i = 0; i < 10; i++) {
        const int16_t effect_num = Effect_Create(room_num);
        if (effect_num == NO_EFFECT) {
            continue;
        }

        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_SPLASH_1;
        effect->pos.x = item->pos.x;
        effect->pos.y = water_height;
        effect->pos.z = item->pos.z;
        effect->rot.y = 2 * Random_GetDraw() + DEG_180;
        effect->speed = Random_GetDraw() / 256;
        effect->frame_num = 0;
    }
}

void Spawn_Ricochet(const GAME_VECTOR *const pos)
{
    const int16_t effect_num = Effect_Create(pos->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_RICOCHET;
        effect->pos = pos->pos;
        effect->counter = 4;
        effect->frame_num = -3 * Random_GetDraw() / 0x8000;
        Sound_Effect(SFX_LARA_RICOCHET, &effect->pos, SPM_NORMAL);
    }
}

void Spawn_Bubble(const XYZ_32 *const pos, const int16_t room_num)
{
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = *pos;
    effect->object_id = O_BUBBLE_1;
    effect->frame_num = -((Random_GetDraw() * 3) / 0x8000);
    effect->speed = 10 + ((Random_GetDraw() * 6) / 0x8000);
}

int16_t Spawn_Blood(
    const int32_t x, const int32_t y, const int32_t z, const int16_t speed,
    const int16_t y_rot, const int16_t room_num)
{
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->pos.x = x;
        effect->pos.y = y;
        effect->pos.z = z;
        effect->rot.y = y_rot;
        effect->speed = speed;
        effect->frame_num = 0;
        effect->object_id = O_BLOOD_1;
        effect->counter = 0;
    }
    return effect_num;
}

void Spawn_BloodBath(
    const int32_t x, const int32_t y, const int32_t z, const int16_t speed,
    const int16_t y_rot, const int16_t room_num, const int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        Spawn_Blood(
            x - (Random_GetDraw() << 9) / 0x8000 + 256,
            y - (Random_GetDraw() << 9) / 0x8000 + 256,
            z - (Random_GetDraw() << 9) / 0x8000 + 256, speed, y_rot, room_num);
    }
}

int16_t Spawn_GunShot(
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
    effect->rot.z = 0;
    effect->rot.x = 0;
    effect->rot.y = y_rot;
    effect->counter = 3;
    effect->frame_num = 0;
    effect->object_id = O_GUN_FLASH;
    effect->shade = SHADE_NEUTRAL;
    return effect_num;
}

int16_t Spawn_GunHit(
    const int32_t x, const int32_t y, const int32_t z, const int16_t speed,
    const int16_t y_rot, const int16_t room_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    XYZ_32 vec = {
        .x = -((Random_GetDraw() - 0x4000) << 7) / 0x7FFF,
        .y = -((Random_GetDraw() - 0x4000) << 7) / 0x7FFF,
        .z = -((Random_GetDraw() - 0x4000) << 7) / 0x7FFF,
    };
    Collide_GetJointAbsPosition(
        lara_item, &vec, Random_GetControl() * LM_NUMBER_OF / 0x7FFF);
    Spawn_Blood(
        vec.x, vec.y, vec.z, lara_item->speed, lara_item->rot.y,
        lara_item->room_num);
    Sound_Effect(SFX_LARA_BULLETHIT, &lara_item->pos, SPM_NORMAL);
    return Spawn_GunShot(x, y, z, speed, y_rot, room_num);
}

int16_t Spawn_GunMiss(
    const int32_t x, const int32_t y, const int32_t z, const int16_t speed,
    const int16_t y_rot, const int16_t room_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    const GAME_VECTOR pos = {
        .x = lara_item->pos.x + ((Random_GetDraw() - 0x4000) << 9) / 0x7FFF,
        .y = lara_item->floor,
        .z = lara_item->pos.z + ((Random_GetDraw() - 0x4000) << 9) / 0x7FFF,
        .room_num = lara_item->room_num,
    };
    Spawn_Ricochet(&pos);
    return Spawn_GunShot(x, y, z, speed, y_rot, room_num);
}
