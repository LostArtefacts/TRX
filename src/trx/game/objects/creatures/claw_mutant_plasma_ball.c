#include <trx/debug.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/creatures/claw_mutant_internal.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

typedef enum {
    M_TYPE_ATTACHED,
    M_TYPE_DETACHED,
} M_TYPE;

static const BITE m_Bite = {
    .pos = { .x = -32, .y = -16, .z = -192 },
    .mesh_num = 13,
};

static const uint8_t m_Falloffs[2] = { 13, 7 };

static int32_t M_GetDamage(void)
{
    OBJECT_PROPERTY_VALUE damage = {};
    const OBJECT *const obj = Object_Get(O_CLAW_MUTANT);
    if (ObjectProperty_GetObjectValue(obj, "plasma_ball_damage", &damage)) {
        return damage.as_int;
    }

    return CLAW_MUTANT_PLASMA_BALL_DAMAGE;
}

static void M_TriggerPlasmaBallFlame(
    const int16_t effect_num, const M_TYPE type, const XYZ_32 vel)
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
    spark->src_color.g = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.b = 255;
    spark->dst_color.r = 32;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->dst_color.b = (Random_GetControl() & 0x3F) + 192;
    spark->fade_to_black = 8;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->life = (Random_GetControl() & 7) + 24;
    spark->s_life = spark->life;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = (Random_GetControl() & 0xF) - 8;
    spark->pos.y = 0;
    spark->pos.z = (Random_GetControl() & 0xF) - 8;
    spark->vel.x = vel.x + (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = vel.y;
    spark->vel.z = vel.z + (Random_GetControl() & 0xFF) - 128;
    spark->friction = 5;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_ROTATE
            | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = +16 + (Random_GetControl() & 0xF);
        }
    } else {
        spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->effect_num = effect_num;
    spark->scalar = 1;
    spark->size.width = (Random_GetControl() & 0x1F) + 64;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = spark->size.width >> 2;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = spark->size.height >> 2;

    if (type == M_TYPE_ATTACHED) {
        spark->scalar = 2;
        spark->vel.x <<= 2;
        spark->vel.y = (Random_GetControl() & 0x1FF) - 256;
        spark->vel.z <<= 2;
        spark->friction = 85;
        spark->dst_size.width >>= 1;
        spark->dst_size.height >>= 1;
    }

    spark->max_y_vel = 0;
    spark->gravity = 0;
    Sparks_FinishSetup(spark);
}

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const XYZ_32 old_pos = effect->pos;
    const M_TYPE type = effect->flag1;

    if (effect->speed < 384 && type == M_TYPE_ATTACHED) {
        effect->speed += (effect->speed >> 3) + 4;
    }

    if (type == M_TYPE_DETACHED) {
        effect->fall_speed++;
        if (effect->speed > 8) {
            effect->speed -= 2;
        }

        if (effect->rot.x > -0x3C00) {
            effect->rot.x -= 0x100;
        }
    }

    const int32_t speed =
        (effect->speed * Math_Cos(effect->rot.x)) >> W2V_SHIFT;
    effect->pos = XYZ_32_OffsetYaw(effect->pos, effect->rot.y, speed);
    effect->pos.y += effect->fall_speed
        - ((effect->speed * Math_Sin(effect->rot.x)) >> W2V_SHIFT);

    const int32_t time4 = Output_GetTimeInGame() * 4;
    if ((time4 & 4) != 0) {
        XYZ_32 vel = {};
        if (type == M_TYPE_DETACHED) {
            vel.y = ABS(old_pos.y - effect->pos.y) << 3;
        }
        M_TriggerPlasmaBallFlame(effect_num, type, vel);
    }

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(effect->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, effect->pos);
    const int32_t ceiling = Room_GetCeiling(sector, effect->pos);

    if (effect->pos.y >= height || effect->pos.y < ceiling) {
        if (type == M_TYPE_ATTACHED && !Room_Get(room_num)->flags.underwater) {
            const int32_t rnd = (Random_GetControl() & 3) + 5;
            for (int32_t i = 0; i < rnd; i++) {
                ClawMutant_TriggerPlasmaBall(
                    nullptr, &old_pos, effect->room_num);
            }
        }

        Effect_Kill(effect_num);
        return;
    }

    if (type == M_TYPE_ATTACHED && Lara_IsNearItem(&effect->pos, 200)) {
        const int32_t rnd = (Random_GetControl() & 1) + 3;
        for (int32_t i = 0; i < rnd; i++) {
            ClawMutant_TriggerPlasmaBall(
                nullptr, &effect->pos, effect->room_num);
        }

        Lara_TakeDamage(M_GetDamage(), true);
        Effect_Kill(effect_num);
        return;
    }

    if (effect->room_num != room_num) {
        Effect_UpdateRoom(effect_num, room_num);
    }

    const int32_t color_base = Random_GetControl();
    const RGB_888 color = {
        .r = color_base & 0x3F,
        .g = 192 - ((color_base >> 6) & 0x1F),
        .b = 255 - ((color_base >> 4) & 0x1F),
    };
    Output_AddDynamicLightRGB(effect->pos, m_Falloffs[type], color);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
}

void ClawMutant_TriggerPlasmaBall(
    const ITEM *const item, const XYZ_32 *const pos, const int16_t room_num)
{
    const M_TYPE type = item == nullptr ? M_TYPE_DETACHED : M_TYPE_ATTACHED;

    XYZ_32 spawn_pos;
    int16_t angles[2];
    int16_t speed;
    if (type == M_TYPE_DETACHED) {
        ASSERT(pos != nullptr);
        spawn_pos = *pos;
        angles[0] = Random_GetControl() << 1;
        angles[1] = DEG_45;
        speed = (Random_GetControl() & 0xF) + 16;
    } else {
        spawn_pos = m_Bite.pos;
        Collide_GetJointAbsPosition(item, &spawn_pos, m_Bite.mesh_num);

        const ITEM *const lara_item = Lara_GetItem();
        Math_GetVectorAngles(
            lara_item->pos.x - spawn_pos.x,
            lara_item->pos.y - spawn_pos.y - STEP_L,
            lara_item->pos.z - spawn_pos.z, angles);
        angles[0] = item->rot.y;
        speed = (Random_GetControl() & 7) + 8;
    }

    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_ITEM) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = spawn_pos;
    effect->rot.x = angles[1];
    effect->rot.y = angles[0];
    effect->object_id = O_CLAW_MUTANT_PLASMA_BALL;
    effect->speed = speed;
    effect->fall_speed = 0;
    effect->flag1 = type;
}

REGISTER_OBJECT(O_CLAW_MUTANT_PLASMA_BALL, M_Setup)
