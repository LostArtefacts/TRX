#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/creatures/claw_mutant_internal.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS    130
#define M_DAMAGE        100
#define M_TOUCH_BITS    0b00000000'10010000
#define M_RADIUS        STEP_L
#define M_ALERT_DIST    SQUARE(WALL_L)         // = 1048576
#define M_ATTACK_DIST_1 SQUARE(WALL_L)         // = 1048576
#define M_ATTACK_DIST_2 SQUARE(WALL_L * 2)     // = 4194304
#define M_ATTACK_DIST_3 SQUARE(WALL_L * 4 / 3) // = 1864135
#define M_FIRE_DIST     SQUARE(WALL_L * 3)     // = 9437184
#define M_WALK_TURN     (DEG_1 * 3)            // = 546
#define M_RUN_TURN      (DEG_1 * 4)            // = 728
#define M_PLASMA_FRAME  28
// clang-format on

typedef enum {
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_RUN_ATTACK,
    M_STATE_WALK_ATTACK_1,
    M_STATE_WALK_ATTACK_2,
    M_STATE_SLASH_LEFT,
    M_STATE_SLASH_RIGHT,
    M_STATE_DEATH,
    M_STATE_CLAW_ATTACK,
    M_STATE_FIRE_ATTACK,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 20,
} M_ANIM;

typedef struct {
    int32_t damage;
    bool recently_fired;
} M_PRIV;

static const BITE m_ClawLeft = {
    .pos = { .x = 19, .y = -13, .z = 3 },
    .mesh_num = 7,
};
static const BITE m_ClawRight = {
    .pos = { .x = 19, .y = -13, .z = 3 },
    .mesh_num = 4,
};
static const BITE m_PlasmaEmitter = {
    .pos = { .x = -32, .y = -16, .z = -192 },
    .mesh_num = 13,
};

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    SHOULD(JSON_READ_OPT(io, "recently_fired", &p->recently_fired));
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "recently_fired", p->recently_fired);
}

static void M_TriggerPlasmaCharge(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const ITEM *const lara_item = Lara_GetItem();

    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
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
    spark->extras = 0;
    spark->life = (Random_GetControl() & 7) + 24;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->friction = 3;
    spark->pos.x = (Random_GetControl() & 0xF) - 8;
    spark->pos.y = 0;
    spark->pos.z = (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0x1F) - 16;
    spark->vel.y = (Random_GetControl() & 0xF) + 16;
    spark->vel.z = (Random_GetControl() & 0x1F) - 16;

    if ((Random_GetControl() & 1) != 0) {
        spark->flags = SPARK_F_ATTACHED_NODE | SPARK_F_ALT_SPRITE | SPARK_F_ITEM
            | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if ((Random_GetControl() & 1) != 0) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_ATTACHED_NODE | SPARK_F_ALT_SPRITE | SPARK_F_ITEM
            | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->gravity = (Random_GetControl() & 0x1F) + 16;
    spark->node_num = 6;
    spark->max_y_vel = (Random_GetControl() & 7) + 16;
    spark->effect_num = item_num;
    spark->scalar = 1;
    spark->size.width = (Random_GetControl() & 0x1F) + 64;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = spark->size.width >> 2;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = spark->size.height >> 2;
    Sparks_FinishSetup(spark);
}

static void M_TriggerLight(const ITEM *const item)
{
    const int16_t frame_idx = Item_GetRelativeFrame(item);
    int32_t scale = 0;
    if (frame_idx > 16) {
        const ANIM *const anim = Item_GetAnim(item);
        const int16_t temp = anim->frame_base - item->frame_num + 44;
        scale = MIN(temp, 16);
    } else {
        scale = frame_idx;
    }

    if (scale <= 0) {
        return;
    }

    const int32_t rnd = Random_GetControl();
    const RGB_888 color = {
        .r = (scale * (rnd & 0x3F)) >> 4,
        .g = (scale * (192 - ((rnd >> 6) & 0x1F))) >> 4,
        .b = (scale * (255 - ((rnd >> 4) & 0x1F))) >> 4,
    };
    XYZ_32 pos = m_PlasmaEmitter.pos;
    Collide_GetJointAbsPosition(item, &pos, m_PlasmaEmitter.mesh_num);
    Output_AddDynamicLightRGB(pos, 13, color);
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;
    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        } else if (Item_TestFrameEqual(item, -1)) {
            Creature_Die(item_num, true);
            for (int32_t i = 0; i < 3; i++) {
                const int32_t dynamic = i == 0 ? -2 : -1;
                Sparks_TriggerExplosionSparks(item->pos, 3, dynamic, 2, 0);
            }
            Sound_Effect(SFX_EXPLOSION_2, &item->pos, SPM_NORMAL);
            return;
        }
        goto finish;
    }

    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    ITEM *const lara_item = Lara_GetItem();
    AI_INFO lara_info = {};
    if (creature->enemy == lara_item) {
        lara_info.angle = info.angle;
        lara_info.distance = info.distance;
    } else {
        const int32_t dx = lara_item->pos.x - item->pos.x;
        const int32_t dz = lara_item->pos.z - item->pos.z;
        lara_info.angle = Math_Atan(dz, dx) - item->rot.y;
        lara_info.distance = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
    }

    const bool violent = info.zone_num == info.enemy_zone_num;
    Creature_Mood(item, &info, violent);
    angle = Creature_Turn(item, creature->maximum_turn);

    ITEM *const enemy = creature->enemy;
    creature->enemy = lara_item;
    if (item->hit_status || lara_info.distance < M_ALERT_DIST
        || Creature_CanSeeEnemy(item, &lara_info)) {
        Creature_AlertAllGuards(item_num);
    }
    creature->enemy = enemy;

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->maximum_turn = 0;
        creature->flags = 0;
        head = info.angle;

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            item->goal_anim_state = M_STATE_STOP;
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
            head = 0;
        } else if (creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (info.bite && info.distance < M_ATTACK_DIST_1) {
            torso_x = info.x_angle;
            torso_y = info.angle;
            if (info.angle < 0) {
                item->goal_anim_state = M_STATE_SLASH_LEFT;
            } else {
                item->goal_anim_state = M_STATE_SLASH_RIGHT;
            }
        } else if (info.bite && info.distance < M_ATTACK_DIST_3) {
            torso_x = info.x_angle;
            torso_y = info.angle;
            item->goal_anim_state = M_STATE_CLAW_ATTACK;
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && ((info.distance > M_FIRE_DIST && !p->recently_fired)
                || info.zone_num != info.enemy_zone_num)) {
            item->goal_anim_state = M_STATE_FIRE_ATTACK;
        } else if (creature->mood == MOOD_BORED) {
            Random_GetControl();
            item->goal_anim_state = M_STATE_WALK;
        } else if (item->required_anim_state) {
            item->goal_anim_state = item->required_anim_state;
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_WALK:
        if (info.ahead) {
            head = info.angle;
        }
        creature->maximum_turn = M_WALK_TURN;

        if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
            head = 0;
        } else if (info.bite && info.distance < M_ATTACK_DIST_3) {
            if (info.angle < 0) {
                item->goal_anim_state = M_STATE_WALK_ATTACK_1;
            } else {
                item->goal_anim_state = M_STATE_WALK_ATTACK_2;
            }
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && ((info.distance > M_FIRE_DIST && !p->recently_fired)
                || info.zone_num != info.enemy_zone_num)) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            creature->mood == MOOD_ESCAPE || creature->mood == MOOD_ATTACK) {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_RUN:
        if (info.ahead) {
            head = info.angle;
        }
        creature->maximum_turn = M_RUN_TURN;

        if ((item->ai_bits & AI_GUARD) != 0 || creature->mood == MOOD_BORED
            || (creature->flags != 0 && info.ahead)) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.bite && info.distance < M_ATTACK_DIST_2) {
            if (lara_item->speed != 0) {
                item->goal_anim_state = M_STATE_RUN_ATTACK;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (
            Creature_CanTargetEnemy(item, &info)
            && ((info.distance > M_FIRE_DIST && !p->recently_fired)
                || info.zone_num != info.enemy_zone_num)) {
            creature->maximum_turn = M_WALK_TURN;
            item->goal_anim_state = M_STATE_STOP;
        }

        creature->flags = 0;
        break;

    case M_STATE_RUN_ATTACK:
    case M_STATE_WALK_ATTACK_1:
    case M_STATE_WALK_ATTACK_2:
    case M_STATE_SLASH_LEFT:
    case M_STATE_SLASH_RIGHT:
    case M_STATE_CLAW_ATTACK:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }

        if (creature->flags == 0 && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Lara_TakeDamage(p->damage, true);
            Creature_Effect(item, &m_ClawLeft, Spawn_Blood);
            Creature_Effect(item, &m_ClawRight, Spawn_Blood);
            creature->flags = 1;
        }

        p->recently_fired = false;
        break;

    case M_STATE_FIRE_ATTACK:
        if (ABS(info.angle) < M_WALK_TURN) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= M_WALK_TURN;
        } else {
            item->rot.y += M_WALK_TURN;
        }

        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle >> 1;
        }

        const int16_t frame_idx = Item_GetRelativeFrame(item);
        if (frame_idx == 0 && (Random_GetControl() & 3) == 0) {
            p->recently_fired = true;
        }

        if (frame_idx < M_PLASMA_FRAME) {
            M_TriggerPlasmaCharge(item_num);
        } else if (frame_idx == M_PLASMA_FRAME) {
            ClawMutant_TriggerPlasmaBall(item, nullptr, item->room_num);
        }
        M_TriggerLight(item);
        break;
    }

finish:
    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_x);
    Creature_Joint(item, 1, torso_y);
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

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    obj->intelligent = true;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 0)->rot.x = true;
    Object_GetBone(obj, 0)->rot.z = true;
    Object_GetBone(obj, 7)->rot.y = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE,
            "Damage dealt by the claw mutant melee attack."),
        OBJECT_PROPERTY_STORED(
            "plasma_ball_damage", CLAW_MUTANT_PLASMA_BALL_DAMAGE,
            "Damage dealt by the claw mutant plasma ball."));
}

REGISTER_OBJECT(O_CLAW_MUTANT, M_Setup)
