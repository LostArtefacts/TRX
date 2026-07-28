#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_RADIUS          (WALL_L / 10)      // = 102
#define M_HIT_POINTS      36
#define M_MAX_FLAME_SPEED 40
#define M_ALERT_DIST      SQUARE(WALL_L)     // = 1048576
#define M_ATTACK_DIST     SQUARE(WALL_L * 4) // = 16777216
#define M_WALK_TURN       (DEG_1 * 5)        // = 910
// clang-format on

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_WAIT,
    M_STATE_SHOOT_1,
    M_STATE_SHOOT_2,
    M_STATE_DEATH,
    M_STATE_AIM_1,
    M_STATE_AIM_2,
    M_STATE_AIM_3,
    M_STATE_SHOOT_3,
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_STOP  = 12,
    M_ANIM_DEATH = 19,
    // clang-format on
} M_ANIM;

static const BITE m_Gun = {
    .pos = { 0, 340, 64 },
    .mesh_num = 7,
};

static void M_TriggerPilotFlame(const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    if (dx < -0x4000 || dx > 0x4000 || dz < -0x4000 || dz > 0x4000) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.g = spark->src_color.r;
    spark->src_color.b = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.r = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 0x80;
    spark->dst_color.b = 32;
    spark->fade_to_black = 4;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 3) + 20;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->pos.x = (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = (Random_GetControl() & 0x1F) - 16;
    spark->pos.z = (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = (Random_GetControl() & 0x1F) - 16;
    spark->vel.y = -(Random_GetControl() & 3);
    spark->vel.z = (Random_GetControl() & 0x1F) - 16;
    spark->flags = SPARK_F_ATTACHED_NODE | SPARK_F_ALT_SPRITE | SPARK_F_ITEM
        | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->effect_num = Item_GetIndex(item);
    spark->node_num = 0;
    spark->friction = 4;
    spark->gravity = -2 - (Random_GetControl() & 3);
    spark->max_y_vel = -4 - (Random_GetControl() & 3);
    spark->scalar = 0;
    spark->dst_size.width = (Random_GetControl() & 7) + 32;
    spark->src_size.width = spark->dst_size.width >> 1;
    spark->size.width = spark->src_size.width;
    spark->src_size.height = spark->src_size.width;
    spark->size.height = spark->src_size.width;
    spark->dst_size.height = spark->dst_size.width;
    Sparks_FinishSetup(spark);
}

static void M_TriggerFlameSparks(
    const XYZ_32 pos, const XYZ_32 vel, const int16_t effect_num)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.g = spark->src_color.r;
    spark->src_color.b = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.r = (Random_GetControl() & 0x3F) - 64;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 0x80;
    spark->dst_color.b = 32;

    if (vel.x != 0 || vel.y != 0 || vel.z != 0) {
        spark->col_fade_speed = 6;
        spark->fade_to_black = 2;
        spark->life = (Random_GetControl() & 1) + 12;
    } else {
        spark->col_fade_speed = 8;
        spark->fade_to_black = 16;
        spark->life = (Random_GetControl() & 3) + 20;
    }

    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->vel.x = vel.x + (Random_GetControl() & 0xF) - 16;
    spark->vel.y = vel.y;
    spark->vel.z = vel.z + (Random_GetControl() & 0xF) - 16;
    spark->friction = 0;

    if ((Random_GetControl() & 1) != 0) {
        if (effect_num < 0) {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
                | SPARK_F_SCALE;
        } else {
            spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_ROTATE
                | SPARK_F_SPRITE | SPARK_F_SCALE;
        }

        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = +16 + (Random_GetControl() & 0xF);
        }
    } else if (effect_num < 0) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    } else {
        spark->flags =
            SPARK_F_ALT_SPRITE | SPARK_F_FX | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->max_y_vel = 0;
    spark->effect_num = effect_num;
    spark->gravity = 0;

    const int32_t size = (Random_GetControl() & 0x1F) + 64;
    if (vel.x != 0 || vel.y != 0 || vel.z != 0) {
        spark->size.width = size >> 5;
        spark->src_size.width = size >> 5;
        spark->size.height = size >> 5;
        spark->src_size.height = size >> 5;

        if (effect_num == -2) {
            spark->scalar = 2;
        } else {
            spark->scalar = 3;
        }

        spark->dst_size.width = size >> 1;
        spark->dst_size.height = size >> 1;
    } else {
        spark->scalar = 4;
        spark->size.width = size >> 4;
        spark->src_size.width = size >> 4;
        spark->size.height = size >> 4;
        spark->src_size.height = size >> 4;
        spark->dst_size.width = size >> 1;
        spark->dst_size.height = size >> 1;
    }
    Sparks_FinishSetup(spark);
}

static void M_TriggerFlamethrower(const ITEM *const item, const int16_t speed)
{
    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    XYZ_32 pos_1 = m_Gun.pos;
    Collide_GetJointAbsPosition(item, &pos_1, m_Gun.mesh_num);

    XYZ_32 pos_2 = m_Gun.pos;
    pos_2.y <<= 1;
    Collide_GetJointAbsPosition(item, &pos_2, m_Gun.mesh_num);

    int16_t angles[2];
    Math_GetVectorAngles(
        pos_2.x - pos_1.x, pos_2.y - pos_1.y, pos_2.z - pos_1.z, angles);

    EFFECT *const effect = Effect_Get(effect_num);
    effect->pos = pos_1;
    effect->rot.x = angles[1];
    effect->rot.y = angles[0];
    effect->speed = speed << 2;
    effect->object_id = O_MISSILE_FLAME;
    effect->counter = 20;
    effect->flag1 = 0;

    M_TriggerFlameSparks((XYZ_32) {}, (XYZ_32) {}, effect_num);

    for (int32_t i = 0; i < 2; i++) {
        const int32_t s = Random_GetControl() % (speed << 2) + 32;
        const int32_t r = (s * Math_Cos(effect->rot.x)) >> W2V_SHIFT;
        const XYZ_32 vel = {
            .x = (r * Math_Sin(effect->rot.y)) >> W2V_SHIFT,
            .y = -((s * Math_Sin(effect->rot.x)) >> W2V_SHIFT),
            .z = (r * Math_Cos(effect->rot.y)) >> W2V_SHIFT,
        };
        M_TriggerFlameSparks(
            effect->pos, (XYZ_32) { vel.x << 5, vel.y << 5, vel.z << 5 }, -1);
    }

    {
        const int32_t r = ((speed << 1) * Math_Cos(effect->rot.x)) >> W2V_SHIFT;
        const XYZ_32 vel = {
            .x = (r * Math_Sin(effect->rot.y)) >> W2V_SHIFT,
            .y = -(((speed << 1) * Math_Sin(effect->rot.x)) >> W2V_SHIFT),
            .z = (r * Math_Cos(effect->rot.y)) >> W2V_SHIFT,
        };
        M_TriggerFlameSparks(
            effect->pos, (XYZ_32) { vel.x << 5, vel.y << 5, vel.z << 5 }, -2);
    }
}

static void M_TriggerLights(const ITEM *const item)
{
    XYZ_32 pos = m_Gun.pos;
    Collide_GetJointAbsPosition(item, &pos, m_Gun.mesh_num);

    const int32_t rnd = Random_GetControl();
    if (item->current_anim_state == M_STATE_SHOOT_2
        || item->current_anim_state == M_STATE_SHOOT_3) {
        const RGB_888 color = {
            .r = 255 - ((rnd >> 4) & 0x1F),
            .g = 192 - ((rnd >> 6) & 0x1F),
            .b = rnd & 0x3F,
        };
        Output_AddDynamicLightRGB(pos, (rnd & 3) + 10, color);
    } else {
        const RGB_888 color = {
            .r = 192 - ((rnd >> 4) & 0x1F),
            .g = 128 - ((rnd >> 6) & 0x1F),
            .b = rnd & 0x1F,
        };
        Output_AddDynamicLightRGB(pos, (rnd & 3) + 6, color);
        M_TriggerPilotFlame(item);
    }
}

static void M_CalculateEnemy(ITEM *const item)
{
    CREATURE *const worker = item->creature_data;
    if (Creature_IsHostile(item)) {
        worker->enemy = Lara_GetItem();
        return;
    }

    worker->enemy = nullptr;
    int32_t best_distance = INT32_MAX;
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM || creature == worker) {
            continue;
        }

        const ITEM *const candidate = Item_Get(creature->item_num);
        if (candidate->object_id == O_LARA
            || candidate->object_id == O_RX_WORKER_2
            || candidate->object_id == O_RX_WORKER_3) {
            continue;
        }

        const int32_t dx = candidate->pos.x - item->pos.x;
        const int32_t dz = candidate->pos.z - item->pos.z;
        const int32_t distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
        if (distance < best_distance) {
            worker->enemy = (ITEM *)candidate;
            best_distance = distance;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    M_TriggerLights(item);

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        }
        goto finish;
    }

    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else {
        M_CalculateEnemy(item);
    }

    // Enforce the following state to avoid Creature_AIInfo resetting ahead,
    // bite and distance when the creature is friendly.
    ITEM *const lara_item = Lara_GetItem();
    const bool hurt_by_lara = creature->hurt_by_lara;
    ITEM *const enemy = creature->enemy;
    if (enemy == nullptr) {
        creature->enemy = lara_item;
        creature->hurt_by_lara = true;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    creature->enemy = enemy;
    creature->hurt_by_lara = hurt_by_lara;

    AI_INFO lara_info = {};
    const bool is_ally = !Creature_IsHostile(item);
    if (creature->enemy == lara_item) {
        lara_info.angle = info.angle;
        lara_info.distance = info.distance;
        if (!creature->hurt_by_lara && is_ally) {
            creature->enemy = nullptr;
        }
    } else {
        const int32_t dx = lara_item->pos.x - item->pos.x;
        const int32_t dz = lara_item->pos.z - item->pos.z;
        info.x_angle -= DEG_45 / 4;
        lara_info.angle = Math_Atan(dz, dx) - item->rot.y;
        lara_info.distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
    }

    Creature_Mood(item, &info, false);
    angle = Creature_Turn(item, creature->maximum_turn);

    if (item->hit_status
        || (!is_ally
            && (lara_info.distance < M_ALERT_DIST
                || Creature_CanSeeEnemy(item, &lara_info)))) {
        if (!creature->alerted) {
            Sound_Effect(SFX_AMERICAN_HOY, &item->pos, SPM_NORMAL);
        }
        Creature_AlertAllGuards(item_num);
    }

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        head = lara_info.angle;
        creature->flags = 0;
        creature->maximum_turn = 0;

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            if ((Random_GetControl() & 0xFF) == 0) {
                item->goal_anim_state = M_STATE_WAIT;
            }
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && (enemy != lara_item || creature->hurt_by_lara || !is_ally)) {
            if (info.distance >= M_ATTACK_DIST) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_AIM_3;
            }
        } else if (
            creature->mood == MOOD_BORED && info.ahead
            && (Random_GetControl() & 0xFF) == 0) {
            item->goal_anim_state = M_STATE_WAIT;
        } else if (
            creature->mood == MOOD_ATTACK
            || (Random_GetControl() & 0xFF) == 0) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WALK:
        head = lara_info.angle;
        creature->flags = 0;
        creature->maximum_turn = M_WALK_TURN;

        if ((item->ai_bits & AI_GUARD) != 0) {
            Item_SwitchToAnim(item, M_ANIM_STOP, 0);
            item->current_anim_state = M_STATE_STOP;
            item->goal_anim_state = M_STATE_STOP;
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && (enemy != lara_item || creature->hurt_by_lara || !is_ally)) {
            if (info.distance >= M_ATTACK_DIST) {
                item->goal_anim_state = M_STATE_AIM_2;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (creature->mood == MOOD_BORED && info.ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WAIT:
        head = lara_info.angle;

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            if ((Random_GetControl() & 0xFF) == 0) {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (
            (Creature_CanTargetEnemy(item, &info)
             && info.distance < M_ATTACK_DIST
             && (enemy != lara_item || creature->hurt_by_lara || !is_ally))
            || creature->mood != MOOD_BORED
            || (Random_GetControl() & 0xFF) == 0) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_AIM_2:
    case M_STATE_AIM_3:
        creature->flags = 0;

        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;

            if (Creature_CanTargetEnemy(item, &info)
                && info.distance < M_ATTACK_DIST
                && (enemy != lara_item || creature->hurt_by_lara || !is_ally)) {
                item->goal_anim_state =
                    item->current_anim_state == M_STATE_AIM_2 ? M_STATE_SHOOT_2
                                                              : M_STATE_SHOOT_3;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
        }
        break;

    case M_STATE_SHOOT_2:
    case M_STATE_SHOOT_3:
        if (creature->flags < M_MAX_FLAME_SPEED) {
            creature->flags += (creature->flags >> 2) + 1;
        }

        const M_STATE stop_state = item->current_anim_state == M_STATE_SHOOT_2
            ? M_STATE_WALK
            : M_STATE_STOP;
        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;

            if (Creature_CanTargetEnemy(item, &info)
                && info.distance < M_ATTACK_DIST
                && (enemy != lara_item || creature->hurt_by_lara || !is_ally)) {
                item->goal_anim_state = item->current_anim_state;
            } else {
                item->goal_anim_state = stop_state;
            }
        } else {
            item->goal_anim_state = stop_state;
        }

        const int16_t speed = creature->flags < M_MAX_FLAME_SPEED
            ? creature->flags
            : ((Random_GetControl() & 0x1F) + 12);
        M_TriggerFlamethrower(item, speed);
        Sound_Effect(SFX_FLAME_THROWER_LOOP, &item->pos, SPM_NORMAL);
        if (enemy != nullptr) {
            const OBJECT *const obj = Object_Get(enemy->object_id);
            if (obj->event_func != nullptr) {
                obj->event_func(enemy, OBJECT_EVENT_BURNT, nullptr);
            }
        }
        break;
    }

finish:
    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 0)->rot.x = true;
    Object_GetBone(obj, 0)->rot.y = true;
    Object_GetBone(obj, 7)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."));
}

REGISTER_OBJECT(O_RX_WORKER_3, M_Setup)
