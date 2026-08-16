#include "willard_internal.h"

#include <trx/core/math/func.h>
#include <trx/game/collision.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

static const uint8_t m_Falloffs[5] = { 13, 7, 7, 7, 7 };

static int32_t M_GetDamage(void)
{
    TRX_VALUE damage = {};
    const OBJECT *const obj = Object_Get(O_WILLARD);
    if (ObjectProperty_GetObjectValue(obj, "plasma_ball_damage", &damage)) {
        return damage.as_int;
    }

    return WILLARD_PLASMA_BALL_DAMAGE;
}

static void M_TriggerPlasmaBallFlame(
    const int16_t effect_num, const int32_t type, const XYZ_32 vel)
{
    const EFFECT *const effect = Effect_Get(effect_num);
    const ITEM *const lara_item = Lara_GetItem();

    const int32_t dx = lara_item->pos.x - effect->pos.x;
    const int32_t dz = lara_item->pos.z - effect->pos.z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 48;
    spark->src_color.g = 255;
    spark->src_color.b = (Random_GetControl() & 0x1F) + 48;
    spark->dst_color.r = 32;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.b = (Random_GetControl() & 0x3F) + 128;
    spark->fade_to_black = 8;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->life = (Random_GetControl() & 7) + 24;
    spark->s_life = spark->life;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->friction = 85;
    spark->pos.x = (Random_GetControl() & 0xF) - 8;
    spark->pos.y = 0;
    spark->pos.z = (Random_GetControl() & 0xF) - 8;
    spark->vel.x = 2 * (vel.x + (Random_GetControl() & 0xFF)) - 256;
    spark->vel.y = (Random_GetControl() & 0x1FF) - 256;
    spark->vel.z = 2 * (vel.z + (Random_GetControl() & 0xFF)) - 256;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_ROTATE
            | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->effect_num = effect_num;
    spark->max_y_vel = 0;
    spark->gravity = 0;

    if (type < 0) {
        if (type >= -2) {
            spark->scalar = 2;
        } else {
            spark->scalar = 4;
        }

        spark->size.width = (Random_GetControl() & 0xF) + 16;
        spark->friction = 5;
        spark->vel.x = vel.x + (Random_GetControl() & 0xFF) - 128;
        spark->vel.y = vel.y;
        spark->vel.z = vel.z + (Random_GetControl() & 0xFF) - 128;
    } else {
        spark->scalar = 3;
        spark->size.width = (uint8_t)type;
    }

    spark->src_size.width = spark->size.width;
    spark->dst_size.width = spark->size.width >> 3;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = spark->size.height >> 3;
    Sparks_FinishSetup(spark);
}

static void M_Control(const int16_t effect_num)
{
    const ITEM *const lara_item = Lara_GetItem();
    EFFECT *const effect = Effect_Get(effect_num);
    const XYZ_32 old_pos = effect->pos;

    const int32_t time4 = Output_GetTimeInGame() * 4;
    if (effect->flag1 != 0) {
        effect->fall_speed += (effect->flag1 != 1) + 1;

        if ((time4 & 0xC) == 0) {
            if (effect->speed != 0) {
                effect->speed--;
            }

            M_TriggerPlasmaBallFlame(
                effect_num, -1 - effect->flag1,
                (XYZ_32) {
                    .x = 0,
                    .y = -(Random_GetControl() & 0x1F),
                    .z = 0,
                });
        }
    } else {
        int16_t angles[2];
        Math_GetVectorAngles(
            lara_item->pos.x - old_pos.x, lara_item->pos.y - old_pos.y - 256,
            lara_item->pos.z - old_pos.z, angles);
        effect->rot.x = angles[1];
        effect->rot.y = angles[0];

        if (effect->speed < 512) {
            effect->speed += (effect->speed >> 4) + 4;
        }

        if ((time4 & 4) != 0) {
            M_TriggerPlasmaBallFlame(
                effect_num, effect->speed >> 1, (XYZ_32) {});
        }
    }

    effect->pos = XYZ_32_Add(
        effect->pos,
        XYZ_32_FromYawPitch(effect->rot.y, effect->rot.x, effect->speed));
    effect->pos.y += effect->fall_speed;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, effect->pos);
    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);

    if (effect->pos.y >= height || effect->pos.y < ceiling) {
        if (effect->flag1 == 0) {
            const int32_t count = (Random_GetControl() & 3) + 2;
            for (int32_t i = 0; i < count; i++) {
                Willard_TriggerPlasmaBall(
                    old_pos, effect->room_num,
                    effect->rot.y + (Random_GetControl() & 0x3FFF) + 0x6000, 1);
            }
        }

        Effect_Destroy(effect_num);
        return;
    }

    if (effect->flag1 == 0 && Lara_IsNearItem(&effect->pos, 200)) {
        for (int32_t i = 14; i >= 0; i -= 2) {
            XYZ_32 pos = { 0, 0, 0 };
            Collide_GetJointAbsPosition(lara_item, &pos, i);
            Willard_TriggerPlasmaBall(
                pos, effect->room_num, Random_GetControl() << 1, 1);
        }

        Lara_CatchFireEx(FLAME_GREEN);
        Lara_TakeDamage(M_GetDamage(), false);
        Effect_Destroy(effect_num);
        return;
    }

    if (effect->room_num != room_num) {
        Effect_UpdateRoom(effect_num, lara_item->room_num);
    }

    const int32_t falloff = m_Falloffs[effect->flag1];
    if (falloff != 0) {
        const int32_t color_base = Random_GetControl();
        Output_AddDynamicLightRGB(
            effect->pos, falloff,
            (RGB_888) {
                .r = color_base & 0x3F,
                .g = 255 - ((color_base >> 4) & 0x1F),
                .b = 192 - ((color_base >> 6) & 0x1F),
            });
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->effect_control_func = M_Control;
    obj->draw_func = nullptr;
}

void Willard_TriggerPlasmaBall(
    const XYZ_32 pos, const int16_t room_num, const int16_t angle,
    const int16_t type)
{
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_ITEM) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = pos;
    effect->rot.x = 0;
    effect->rot.y = angle;
    effect->object_id = O_WILLARD_PLASMA_BALL;
    effect->speed = type != -16 ? (Random_GetControl() & 0x1F) + 16 : 0;
    effect->fall_speed = -16 * type;
    effect->flag1 = type;
}

REGISTER_OBJECT(O_WILLARD_PLASMA_BALL, M_Setup)
