#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/creature.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS     50
#define M_DAMAGE         50
#define M_RADIUS         (WALL_L / 10) // = 102
#define M_WALK_TURN      (DEG_1 * 4) // = 728
#define M_RUN_TURN       (DEG_1 * 6) // = 1092
#define M_WALK_RANGE     SQUARE(WALL_L * 2) // = 4194304
#define M_WALK_CHANCE    0x4000
#define M_SHOOT_1_CHANCE 0x2000
#define M_SHOOT_2_CHANCE 0x5000
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_AIM_4,
    M_STATE_WAIT,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_AIM_1,
    M_STATE_AIM_2,
    M_STATE_SHOOT_1,
    M_STATE_SHOOT_2,
    M_STATE_SHOOT_4A,
    M_STATE_SHOOT_4B,
    M_STATE_DEATH,
    M_STATE_AIM_5,
    M_STATE_SHOOT_5,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 9,
} M_ANIM;

static const CREATURE_GUN m_Bandit2Gun = {
    .muzzle = { .pos = { .x = -1, .y = 230, .z = 9 }, .mesh_num = 17 },
};

static int32_t M_GetShootDamage(const ITEM *const item)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DAMAGE;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;

    int16_t head = 0;
    int16_t tilt = 0;
    int16_t neck = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_WAIT:
            if (info.ahead) {
                neck = info.angle;
            }
            creature->maximum_turn = 0;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance > M_WALK_RANGE
                    && Random_GetControl() < M_WALK_CHANCE) {
                    item->goal_anim_state = M_STATE_WALK;
                } else {
                    const int32_t random = Random_GetControl();
                    if (random < M_SHOOT_1_CHANCE) {
                        item->goal_anim_state = M_STATE_SHOOT_1;
                    } else if (random < M_SHOOT_2_CHANCE) {
                        item->goal_anim_state = M_STATE_SHOOT_2;
                    } else {
                        item->goal_anim_state = M_STATE_AIM_5;
                    }
                }
            } else if (creature->mood == MOOD_BORED) {
                if (!info.ahead || Random_GetControl() < 0x100) {
                    item->goal_anim_state = M_STATE_WALK;
                }
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_WALK:
            if (info.ahead) {
                neck = info.angle;
            }
            creature->maximum_turn = M_WALK_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance < M_WALK_RANGE
                    || info.zone_num == info.enemy_zone_num
                    || Random_GetControl() < 0x400) {
                    item->goal_anim_state = M_STATE_WAIT;
                } else {
                    item->goal_anim_state = M_STATE_AIM_4;
                }
            } else if (creature->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.ahead && Random_GetControl() < 0x400) {
                item->goal_anim_state = M_STATE_WAIT;
            }
            break;

        case M_STATE_RUN:
            if (info.ahead) {
                neck = info.angle;
            }
            creature->maximum_turn = M_RUN_TURN;
            tilt = angle / 2;
            if (creature->mood == MOOD_ESCAPE) {
            } else if (
                creature->mood == MOOD_BORED
                || Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_WAIT;
            }
            break;

        case M_STATE_AIM_1:
        case M_STATE_AIM_2:
        case M_STATE_AIM_4:
            if (info.ahead) {
                head = info.angle;
            }
            creature->flags = 0;
            break;

        case M_STATE_AIM_5:
            if (info.ahead) {
                head = info.angle;
            }
            creature->flags = 0;
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_SHOOT_5;
            } else {
                item->goal_anim_state = M_STATE_WAIT;
            }
            break;

        case M_STATE_SHOOT_1:
        case M_STATE_SHOOT_2:
        case M_STATE_SHOOT_5:
            if (info.ahead) {
                head = info.angle;
            }
            if (creature->flags == 0) {
                if (!Creature_Shoot(
                        item, &info, &m_Bandit2Gun, head,
                        M_GetShootDamage(item))
                    || Random_GetControl() < 0x2000) {
                    item->goal_anim_state = M_STATE_WAIT;
                }
                creature->flags = 1;
            }
            break;

        case M_STATE_SHOOT_4A:
            if (info.ahead) {
                head = info.angle;
            }
            if (creature->flags != 1) {
                if (!Creature_Shoot(
                        item, &info, &m_Bandit2Gun, head,
                        M_GetShootDamage(item))) {
                    item->goal_anim_state = M_STATE_WALK;
                }
                creature->flags = 1;
            }
            if (info.distance < M_WALK_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        case M_STATE_SHOOT_4B:
            if (info.ahead) {
                head = info.angle;
            }
            if (creature->flags != 2) {
                if (!Creature_Shoot(
                        item, &info, &m_Bandit2Gun, head,
                        M_GetShootDamage(item))) {
                    item->goal_anim_state = M_STATE_WALK;
                }
                creature->flags = 2;
            }
            if (info.distance < M_WALK_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        default:
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Neck(item, neck);
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup2A(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 6)->rot.y = true;
    Object_GetBone(obj, 8)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_DAMAGE, "Damage dealt by the bandit's shot."));
}

static void M_Setup2B(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    const OBJECT *const ref_obj = Object_Get(O_BANDIT_2);
    if (ref_obj->loaded) {
        obj->anim_idx = ref_obj->anim_idx;
        obj->frame_base = ref_obj->frame_base;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 6)->rot.y = true;
    Object_GetBone(obj, 8)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_DAMAGE, "Damage dealt by the bandit's shot."));
}

REGISTER_OBJECT(O_BANDIT_2, M_Setup2A)
REGISTER_OBJECT(O_BANDIT_2B, M_Setup2B)
