#include "game/effects.h"
#include "game/lara.h"
#include "game/math.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "utils.h"

#define SHARD_DAMAGE 30
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE_BASE WALL_L
#define ROCKET_RANGE SQUARE(ROCKET_RANGE_BASE) // = 1048576

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const ITEM *const lara_item = Lara_GetItem();

    const int32_t speed =
        (effect->speed * Math_Cos(effect->rot.x)) >> W2V_SHIFT;
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
            effect->object_id = O_RICOCHET;
            effect->frame_num = -Random_GetControl() / 11000;
            effect->speed = 0;
            effect->counter = 6;
            Sound_Effect(SFX_LARA_RICOCHET, &effect->pos, SPM_NORMAL);
        } else {
            effect->object_id = O_EXPLOSION_1;
            effect->frame_num = 0;
            effect->speed = 0;
            effect->counter = 0;
            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);

            const int32_t x = effect->pos.x - lara_item->pos.x;
            const int32_t y = effect->pos.y - lara_item->pos.y;
            const int32_t z = effect->pos.z - lara_item->pos.z;
            if (Item_Test3DRange(x, y, z, ROCKET_RANGE_BASE)) {
                const int32_t range = SQUARE(x) + SQUARE(y) + SQUARE(z);
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
        if (lara_item->hit_points > 0) {
            Sound_Effect(SFX_LARA_INJURY, &lara_item->pos, SPM_NORMAL);
            LARA_INFO *const lara = Lara_GetLaraInfo();
            lara->hit_effect = effect;
            lara->hit_effect_count = 5;
        }
        Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
    }

    effect->frame_num = 0;
    effect->rot.y = lara_item->rot.y;
    effect->speed = lara_item->speed;
    effect->counter = 0;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

REGISTER_OBJECT(O_MISSILE_2, M_Setup)
REGISTER_OBJECT(O_MISSILE_3, M_Setup)
