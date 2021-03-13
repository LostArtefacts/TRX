#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/vars.h"
#include "game/warrior.h"

#define SHARD_DAMAGE 30
#define SHARD_SPEED 250
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE SQUARE(WALL_L) // = 1048576
#define ROCKET_SPEED 220

void ControlMissile(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];

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
            fx->frame_number = -GetRandomControl() / 11000;
            fx->speed = 0;
            fx->counter = 6;
            SoundEffect(SFX_LARA_RICOCHET, &fx->pos, SPM_NORMAL);
        } else {
            fx->object_number = O_EXPLOSION1;
            fx->frame_number = 0;
            fx->speed = 0;
            fx->counter = 0;
            SoundEffect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);

            int32_t x = fx->pos.x - LaraItem->pos.x;
            int32_t y = fx->pos.y - LaraItem->pos.y;
            int32_t z = fx->pos.z - LaraItem->pos.z;
            int32_t range = SQUARE(x) + SQUARE(y) + SQUARE(z);
            if (range < ROCKET_RANGE) {
                LaraItem->hit_points -= (int16_t)(
                    ROCKET_DAMAGE * (ROCKET_RANGE - range) / ROCKET_RANGE);
                LaraItem->hit_status = 1;
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
        LaraItem->hit_points -= SHARD_DAMAGE;
        fx->object_number = O_BLOOD1;
        SoundEffect(SFX_LARA_BULLETHIT, &fx->pos, SPM_NORMAL);
    } else {
        LaraItem->hit_points -= ROCKET_DAMAGE;
        fx->object_number = O_EXPLOSION1;
        if (LaraItem->hit_points > 0) {
            SoundEffect(SFX_LARA_INJURY, &LaraItem->pos, SPM_NORMAL);
            Lara.spaz_effect = fx;
            Lara.spaz_effect_count = 5;
        }
        SoundEffect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
    }
    LaraItem->hit_status = 1;

    fx->frame_number = 0;
    fx->pos.y_rot = LaraItem->pos.y_rot;
    fx->speed = LaraItem->speed;
    fx->counter = 0;
}

void ShootAtLara(FX_INFO *fx)
{
    int32_t x = LaraItem->pos.x - fx->pos.x;
    int32_t y = LaraItem->pos.y - fx->pos.y;
    int32_t z = LaraItem->pos.z - fx->pos.z;

    int16_t *bounds = GetBoundsAccurate(LaraItem);
    y += bounds[FRAME_BOUND_MAX_Y]
        + (bounds[FRAME_BOUND_MIN_Y] - bounds[FRAME_BOUND_MAX_Y]) * 3 / 4;

    int32_t dist = phd_sqrt(SQUARE(x) + SQUARE(z));
    fx->pos.x_rot = -(PHD_ANGLE)phd_atan(dist, y);
    fx->pos.y_rot = phd_atan(z, x);
    fx->pos.x_rot += (GetRandomControl() - 0x4000) / 0x40;
    fx->pos.y_rot += (GetRandomControl() - 0x4000) / 0x40;
}

int16_t ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &Effects[fx_num];
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
        FX_INFO *fx = &Effects[fx_num];
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

int32_t ExplodingDeath(int16_t item_num, int32_t mesh_bits, int16_t damage)
{
    ITEM_INFO *item = &Items[item_num];
    OBJECT_INFO *object = &Objects[item->object_number];
    int32_t abortion = item->object_number == O_ABORTION;

    int16_t *frame = GetBestFrame(item);

    phd_PushUnitMatrix();
    PhdMatrixPtr->_03 = 0;
    PhdMatrixPtr->_13 = 0;
    PhdMatrixPtr->_23 = 0;

    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
    phd_RotYXZpack(*packed_rotation++);

    int32_t *bone = &AnimBones[object->bone_index];
#if 0
    // XXX: present in OG, removed by GLrage on the grounds that it sometimes
    // crashes.
    int16_t *extra_rotation = (int16_t*)item->data;
#endif

    int32_t bit = 1;
    if ((bit & mesh_bits) && (bit & item->mesh_bits)) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->room_number = item->room_number;
            fx->pos.x = (PhdMatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
            fx->pos.y = (PhdMatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
            fx->pos.z = (PhdMatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
            fx->pos.y_rot = (GetRandomControl() - 0x4000) * 2;
            if (abortion) {
                fx->speed = GetRandomControl() >> 7;
                fx->fall_speed = -GetRandomControl() >> 7;
            } else {
                fx->speed = GetRandomControl() >> 8;
                fx->fall_speed = -GetRandomControl() >> 8;
            }
            fx->counter = damage;
            fx->frame_number = object->mesh_index;
            fx->object_number = O_BODY_PART;
        }
        item->mesh_bits -= bit;
    }

    for (int i = 1; i < object->nmeshes; i++) {
        int32_t bone_extra_flags = *bone++;
        if (bone_extra_flags & BEB_POP) {
            phd_PopMatrix();
        }
        if (bone_extra_flags & BEB_PUSH) {
            phd_PushMatrix();
        }

        phd_TranslateRel(bone[0], bone[1], bone[2]);
        phd_RotYXZpack(*packed_rotation++);

#if 0
    if (extra_rotation) {
        if (bone_extra_flags & (BEB_ROT_X | BEB_ROT_Y | BEB_ROT_Z)) {
            if (bone_extra_flags & BEB_ROT_Y) {
                phd_RotY(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                phd_RotX(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                phd_RotZ(*extra_rotation++);
            }
        }
    }
#endif

        bit <<= 1;
        if ((bit & mesh_bits) && (bit & item->mesh_bits)) {
            int16_t fx_num = CreateEffect(item->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &Effects[fx_num];
                fx->room_number = item->room_number;
                fx->pos.x = (PhdMatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
                fx->pos.y = (PhdMatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
                fx->pos.z = (PhdMatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
                fx->pos.y_rot = (GetRandomControl() - 0x4000) * 2;
                if (abortion) {
                    fx->speed = GetRandomControl() >> 7;
                    fx->fall_speed = -GetRandomControl() >> 7;
                } else {
                    fx->speed = GetRandomControl() >> 8;
                    fx->fall_speed = -GetRandomControl() >> 8;
                }
                fx->counter = damage;
                fx->object_number = O_BODY_PART;
                fx->frame_number = object->mesh_index + i;
            }
            item->mesh_bits -= bit;
        }

        bone += 3;
    }

    phd_PopMatrix();

    return !(item->mesh_bits & (0x7FFFFFFF >> (31 - object->nmeshes)));
}

void ControlBodyPart(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->pos.x_rot += 5 * PHD_DEGREE;
    fx->pos.z_rot += 10 * PHD_DEGREE;
    fx->pos.z += (fx->speed * phd_cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * phd_sin(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);

    int32_t ceiling = GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y < ceiling) {
        fx->fall_speed = -fx->fall_speed;
        fx->pos.y = ceiling;
    }

    int32_t height = GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z);
    if (fx->pos.y >= height) {
        if (fx->counter) {
            fx->speed = 0;
            fx->frame_number = 0;
            fx->counter = 0;
            fx->object_number = O_EXPLOSION1;
            SoundEffect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);
        } else {
            KillEffect(fx_num);
        }
        return;
    }

    if (ItemNearLara(&fx->pos, fx->counter * 2)) {
        LaraItem->hit_points -= fx->counter;
        LaraItem->hit_status = 1;

        if (fx->counter) {
            fx->speed = 0;
            fx->frame_number = 0;
            fx->counter = 0;
            fx->object_number = O_EXPLOSION1;
            SoundEffect(SFX_ATLANTEAN_EXPLODE, &fx->pos, SPM_NORMAL);

            Lara.spaz_effect_count = 5;
            Lara.spaz_effect = fx;
        } else {
            KillEffect(fx_num);
        }
    }

    if (room_num != fx->room_number) {
        EffectNewRoom(fx_num, room_num);
    }
}
