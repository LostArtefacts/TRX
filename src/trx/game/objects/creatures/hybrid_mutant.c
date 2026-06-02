#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_RADIUS           (WALL_L / 8)           // = 128
#define M_HIT_POINTS       90
#define M_SLASH_DAMAGE     100
#define M_KICK_DAMAGE      80
#define M_JUMP_DAMAGE      20
#define M_TOUCH_BITS_LEFT  0b00000000'10000000
#define M_TOUCH_BITS_RIGHT 0b00001000'00000000
#define M_ALERT_DIST       SQUARE(WALL_L)         // = 1048576
#define M_ATTACK_DIST_1    SQUARE(WALL_L)         // = 1048576
#define M_ATTACK_DIST_2    SQUARE(WALL_L * 2)     // = 4194304
#define M_ATTACK_DIST_3    SQUARE(WALL_L * 4 / 3) // = 1863225
#define M_JUMP_ANGLE       DEG_45
#define M_WALK_TURN        (DEG_1 * 3)            // = 546
#define M_RUN_TURN         (DEG_1 * 6)            // = 1092
// clang-format on

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_JUMP_START,
    M_STATE_JUMP_MID,
    M_STATE_JUMP_END,
    M_STATE_SLASH,
    M_STATE_KICK,
    M_STATE_RUN_ATTACK,
    M_STATE_WALK_ATTACK,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 18,
} M_ANIM;

static const BITE m_BiteLeft = {
    .pos = { 19, -13, 3 },
    .mesh_num = 7,
};
static const BITE m_BiteRight = {
    .pos = { 19, -13, 3 },
    .mesh_num = 14,
};

static int32_t M_GetDamage(
    const ITEM *const item, const char *const key, const int32_t default_value)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, key, &damage)) {
        return damage.as_int;
    }

    return default_value;
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

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
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

    Creature_Mood(item, &info, true);
    angle = Creature_Turn(item, creature->maximum_turn);

    ITEM *const enemy = creature->enemy;
    creature->enemy = lara_item;
    if ((lara_info.distance < M_ALERT_DIST || item->hit_status
         || Creature_CanSeeEnemy(item, &lara_info))) {
        Creature_AlertAllGuards(item_num);
    }
    creature->enemy = enemy;

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    switch (item->current_anim_state) {
    case M_STATE_STOP:
        head = info.angle;
        creature->maximum_turn = 0;
        creature->flags = 0;

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(creature);
            item->goal_anim_state = M_STATE_STOP;
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
            head = 0;
        } else if (creature->mood == MOOD_ESCAPE) {
            if (lara->target == item || !info.ahead) {
                item->goal_anim_state = M_STATE_RUN;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (
            info.angle < M_JUMP_ANGLE && info.angle > -M_JUMP_ANGLE
            && info.distance > M_ATTACK_DIST_1) {
            item->goal_anim_state = M_STATE_JUMP_START;
        } else if (info.bite && info.distance < M_ATTACK_DIST_1) {
            torso_x = info.x_angle;
            torso_y = info.angle;
            if (info.angle < 0) {
                item->goal_anim_state = M_STATE_SLASH;
            } else {
                item->goal_anim_state = M_STATE_KICK;
            }
        } else if (item->required_anim_state != M_STATE_NULL) {
            item->goal_anim_state = item->required_anim_state;
        } else if (info.distance < M_ATTACK_DIST_2) {
            item->goal_anim_state = M_STATE_WALK;
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
            creature->maximum_turn = M_WALK_TURN;
            item->goal_anim_state = M_STATE_WALK_ATTACK;
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

        if ((item->ai_bits & AI_GUARD) != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            creature->mood == MOOD_BORED
            || (creature->mood == MOOD_ESCAPE && lara->target != item
                && info.ahead)) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->flags != 0 && info.ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.bite && info.distance < M_ATTACK_DIST_2) {
            if (lara_item->speed != 0) {
                item->goal_anim_state = M_STATE_RUN_ATTACK;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (info.distance < M_ATTACK_DIST_2) {
            item->goal_anim_state = M_STATE_WALK;
        }

        creature->flags = 0;
        break;

    case M_STATE_JUMP_START:
        creature->maximum_turn = M_WALK_TURN;
        break;

    case M_STATE_JUMP_MID:
    case M_STATE_JUMP_END:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }
        creature->maximum_turn = 0;

        if ((item->touch_bits & M_TOUCH_BITS_LEFT) != 0) {
            Lara_TakeDamage(
                M_GetDamage(item, "jump_damage", M_JUMP_DAMAGE), true);
            Creature_Effect(item, &m_BiteLeft, Spawn_Blood);
        }
        break;

    case M_STATE_SLASH:
    case M_STATE_RUN_ATTACK:
    case M_STATE_WALK_ATTACK:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }
        creature->maximum_turn = M_WALK_TURN;

        if (creature->flags == 0
            && (item->touch_bits & M_TOUCH_BITS_LEFT) != 0) {
            Lara_TakeDamage(
                M_GetDamage(item, "slash_damage", M_SLASH_DAMAGE), true);
            Creature_Effect(item, &m_BiteLeft, Spawn_Blood);
            creature->flags = 1;
        }

        if (!info.bite || info.distance >= M_ATTACK_DIST_1) {
            item->goal_anim_state = M_STATE_STOP;
        }

        if (Item_TestFrameEqual(item, -1)) {
            creature->flags = 0;
        }
        break;

    case M_STATE_KICK:
        if (info.ahead) {
            torso_x = info.x_angle;
            torso_y = info.angle;
        }
        creature->maximum_turn = M_WALK_TURN;

        if (creature->flags == 0
            && (item->touch_bits & M_TOUCH_BITS_RIGHT) != 0) {
            Lara_TakeDamage(
                M_GetDamage(item, "kick_damage", M_KICK_DAMAGE), true);
            Creature_Effect(item, &m_BiteRight, Spawn_Blood);
            creature->flags = 1;
        }

        if (!info.bite || info.distance >= M_ATTACK_DIST_1) {
            item->goal_anim_state = M_STATE_STOP;
        }

        if (Item_TestFrameEqual(item, -1)) {
            creature->flags = 0;
        }
        break;
    }

finish:
    Creature_Tilt(item, 0);
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

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 0)->rot.x = true;
    Object_GetBone(obj, 0)->rot.z = true;
    Object_GetBone(obj, 7)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "jump_damage", M_JUMP_DAMAGE,
            "Damage dealt by the hybrid mutant jump attack."),
        OBJECT_PROPERTY_INT(
            "slash_damage", M_SLASH_DAMAGE,
            "Damage dealt by the hybrid mutant slash attack."),
        OBJECT_PROPERTY_INT(
            "kick_damage", M_KICK_DAMAGE,
            "Damage dealt by the hybrid mutant kick attack."));
}

REGISTER_OBJECT(O_HYBRID_MUTANT, M_Setup)
