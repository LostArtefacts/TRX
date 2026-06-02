#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

// clang-format off
#define M_HIT_POINTS    (g_TRVersion == 1 ? 20 : 30)
#define M_CHARGE_DAMAGE 3
#define M_SLAM_DAMAGE   200
#define M_ATTACK_DAMAGE 200
#define M_PAT_DAMAGE    400
#define M_TOUCH         0x2406C
#define M_ROAR_CHANCE   80
#define M_REAR_CHANCE   768
#define M_DROP_CHANCE   1536
#define M_RADIUS        (WALL_L / 3) // = 341
#define M_REAR_RANGE    SQUARE(WALL_L * 2) // = 4194304
#define M_ATTACK_RANGE  SQUARE(WALL_L) // = 1048576
#define M_PAT_RANGE     SQUARE(600) // = 360000
#define M_FIX_PAT_RANGE SQUARE(300) // = 90000
#define M_RUN_TURN      (5 * DEG_1) // = 910
#define M_WALK_TURN     (2 * DEG_1) // = 364
#define M_EAT_RANGE     SQUARE(WALL_L * 3 / 4) // = 589824
// clang-format on

typedef enum {
    M_STATE_STROLL,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_REAR,
    M_STATE_ROAR,
    M_STATE_ATTACK_1,
    M_STATE_ATTACK_2,
    M_STATE_EAT,
    M_STATE_DEATH,
} M_STATE;

static BITE m_BearHeadBite = {
    .pos = { 0, 96, 335 },
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
    OBJECT *const obj = Object_Get(item->object_id);
    obj->pivot_length = g_Config.gameplay.fix_bear_ai ? 0 : 500;

    CREATURE *const bear = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        angle = Creature_Turn(item, DEG_1);

        switch (item->current_anim_state) {
        case M_STATE_WALK:
            item->goal_anim_state = M_STATE_REAR;
            break;

        case M_STATE_RUN:
        case M_STATE_STROLL:
            item->goal_anim_state = M_STATE_STOP;
            break;

        case M_STATE_REAR:
            bear->flags = 1;
            item->goal_anim_state = M_STATE_DEATH;
            break;

        case M_STATE_STOP:
            bear->flags = 0;
            item->goal_anim_state = M_STATE_DEATH;
            break;

        case M_STATE_DEATH:
            if (bear != nullptr && bear->flags != 0
                && (item->touch_bits & M_TOUCH) != 0) {
                Lara_TakeDamage(
                    M_GetDamage(item, "slam_damage", M_SLAM_DAMAGE), true);
                bear->flags = 0;
            }
            break;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, bear->maximum_turn);

        const bool dead_enemy = Lara_GetItem()->hit_points <= 0;
        if (item->hit_status) {
            bear->flags = 1;
        }

        switch ((int16_t)item->current_anim_state) {
        case M_STATE_STOP:
            if (dead_enemy) {
                if (info.bite && info.distance < M_EAT_RANGE) {
                    item->goal_anim_state = M_STATE_EAT;
                } else {
                    item->goal_anim_state = M_STATE_STROLL;
                }
            } else if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (bear->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_STROLL;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_STROLL:
            bear->maximum_turn = M_WALK_TURN;
            if (dead_enemy && (item->touch_bits & M_TOUCH) && info.ahead) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (bear->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
                if (bear->mood == MOOD_ESCAPE) {
                    item->required_anim_state = M_STATE_STROLL;
                }
            } else if (Random_GetControl() < M_ROAR_CHANCE) {
                item->required_anim_state = M_STATE_ROAR;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_RUN:
            bear->maximum_turn = M_RUN_TURN;
            if (item->touch_bits & M_TOUCH) {
                Lara_TakeDamage(
                    M_GetDamage(item, "charge_damage", M_CHARGE_DAMAGE), true);
            }
            if (bear->mood == MOOD_BORED || dead_enemy) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.ahead && !item->required_anim_state) {
                if (!bear->flags && info.distance < M_REAR_RANGE
                    && Random_GetControl() < M_REAR_CHANCE) {
                    item->required_anim_state = M_STATE_REAR;
                    item->goal_anim_state = M_STATE_STOP;
                } else if (info.distance < M_ATTACK_RANGE) {
                    item->goal_anim_state = M_STATE_ATTACK_1;
                }
            }
            break;

        case M_STATE_REAR:
            if (bear->flags) {
                item->required_anim_state = M_STATE_STROLL;
                item->goal_anim_state = M_STATE_STOP;
            } else if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (bear->mood == MOOD_BORED || bear->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                info.bite
                && info.distance
                    < (g_Config.gameplay.fix_bear_ai ? M_FIX_PAT_RANGE
                                                     : M_PAT_RANGE)) {
                item->goal_anim_state = M_STATE_ATTACK_2;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        case M_STATE_WALK:
            if (bear->flags) {
                item->required_anim_state = M_STATE_STROLL;
                item->goal_anim_state = M_STATE_REAR;
            } else if (info.ahead && (item->touch_bits & M_TOUCH)) {
                item->goal_anim_state = M_STATE_REAR;
            } else if (bear->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_REAR;
                item->required_anim_state = M_STATE_STROLL;
            } else if (
                bear->mood == MOOD_BORED
                || Random_GetControl() < M_ROAR_CHANCE) {
                item->required_anim_state = M_STATE_ROAR;
                item->goal_anim_state = M_STATE_REAR;
            } else if (
                info.distance > M_REAR_RANGE
                || Random_GetControl() < M_DROP_CHANCE) {
                item->required_anim_state = M_STATE_STOP;
                item->goal_anim_state = M_STATE_REAR;
            }
            break;

        case M_STATE_ATTACK_1:
            if (!item->required_anim_state && (item->touch_bits & M_TOUCH)) {
                Creature_Effect(item, &m_BearHeadBite, Spawn_Blood);
                Lara_TakeDamage(
                    M_GetDamage(item, "attack_damage", M_ATTACK_DAMAGE), true);
                item->required_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_ATTACK_2:
            if (!item->required_anim_state && (item->touch_bits & M_TOUCH)) {
                Lara_TakeDamage(
                    M_GetDamage(item, "pat_damage", M_PAT_DAMAGE), true);
                item->required_anim_state = M_STATE_REAR;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->radius = M_RADIUS;
    obj->smartness = 0x4000;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 13)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "charge_damage", M_CHARGE_DAMAGE,
            "Damage dealt while the bear charges into Lara."),
        OBJECT_PROPERTY_INT(
            "slam_damage", M_SLAM_DAMAGE,
            "Damage dealt if the falling bear slams into Lara."),
        OBJECT_PROPERTY_INT(
            "attack_damage", M_ATTACK_DAMAGE,
            "Damage dealt by the bear bite attack."),
        OBJECT_PROPERTY_INT(
            "pat_damage", M_PAT_DAMAGE, "Damage dealt by the bear paw swipe."));
}

REGISTER_OBJECT(O_BEAR, M_Setup)
