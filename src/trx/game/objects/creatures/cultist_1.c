#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/creature.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>

// clang-format off
#define M_HIT_POINTS    25
#define M_DAMAGE        50
#define M_RADIUS        (WALL_L / 10) // = 102
#define M_WALK_TURN     (DEG_1 * 5) // = 910
#define M_RUN_TURN      (DEG_1 * 5) // = 910
#define M_RUN_RANGE     SQUARE(WALL_L * 2) // = 4194304
#define M_POSE_CHANCE   0x500 // = 1280
#define M_UNPOSE_CHANCE 0x100 // = 256
#define M_WALK_CHANCE   (M_POSE_CHANCE + 0x500) // = 2560
#define M_UNWALK_CHANCE 0x300 // = 768
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_STOP,
    M_STATE_WAIT_1,
    M_STATE_WAIT_2,
    M_STATE_AIM_1,
    M_STATE_SHOOT_1,
    M_STATE_AIM_2,
    M_STATE_SHOOT_2,
    M_STATE_AIM_3,
    M_STATE_SHOOT_3,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 20,
} M_ANIM;

static const CREATURE_GUN m_Cultist1Gun = {
    .muzzle = { .pos = { .x = 3, .y = 331, .z = 56 }, .mesh_num = 10 },
};

static int32_t M_GetShootDamage(const ITEM *const item)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DAMAGE;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Random_GetControl() < 0x4000) {
        item->mesh_bits &= ~0b00110000;
    }
    if (item->object_id == O_CULT_1B) {
        // clang-format off
        // TODO: clang-format >=20 formats this wrongly
        item->mesh_bits &= ~0b00011111'10000000'00000000;
        // clang-format on
    }
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
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(
                item, Random_GetControl() / 0x4000 + M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            creature->maximum_turn = 0;
            if (item->required_anim_state != M_STATE_EMPTY) {
                item->goal_anim_state = item->required_anim_state;
            }
            break;

        case M_STATE_WAIT_1:
            if (creature->mood == MOOD_ESCAPE) {
                item->required_anim_state = M_STATE_RUN;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_STOP;
                item->required_anim_state = Random_GetControl() < 0x4000
                    ? M_STATE_AIM_1
                    : M_STATE_AIM_3;
            } else if (creature->mood == MOOD_BORED && info.ahead) {
                const int16_t random = Random_GetControl();
                if (random < M_POSE_CHANCE) {
                    item->required_anim_state = M_STATE_WAIT_2;
                    item->goal_anim_state = M_STATE_STOP;
                } else if (random < M_WALK_CHANCE) {
                    item->required_anim_state = M_STATE_WALK;
                    item->goal_anim_state = M_STATE_STOP;
                }
            } else if (
                creature->mood == MOOD_BORED || info.distance < M_RUN_RANGE) {
                item->required_anim_state = M_STATE_WALK;
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->required_anim_state = M_STATE_RUN;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_WAIT_2:
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_STOP;
                item->required_anim_state = M_STATE_AIM_1;
            } else if (
                creature->mood != MOOD_BORED
                || Random_GetControl() < M_UNPOSE_CHANCE || !info.ahead) {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_WALK:
            creature->maximum_turn = M_WALK_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_STOP;
                item->required_anim_state = Random_GetControl() < 0x4000
                    ? M_STATE_AIM_1
                    : M_STATE_AIM_3;
            } else if (info.distance > M_RUN_RANGE || !info.ahead) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (
                creature->mood == MOOD_BORED && info.ahead
                && Random_GetControl() < M_UNWALK_CHANCE) {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_RUN:
            creature->maximum_turn = M_RUN_TURN;
            creature->flags = 0;
            tilt = angle / 4;
            if (creature->mood == MOOD_ESCAPE) {
                if (Creature_CanTargetEnemy(item, &info)) {
                    item->goal_anim_state = M_STATE_SHOOT_2;
                }
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance < M_RUN_RANGE
                    || info.zone_num != info.enemy_zone_num) {
                    item->goal_anim_state = M_STATE_STOP;
                } else {
                    item->goal_anim_state = M_STATE_SHOOT_2;
                }
            } else if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_AIM_1:
        case M_STATE_AIM_3:
            creature->flags = 0;
            if (info.ahead) {
                head = info.angle;
            }
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state =
                    item->current_anim_state == M_STATE_AIM_1 ? M_STATE_SHOOT_1
                                                              : M_STATE_SHOOT_3;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_SHOOT_1:
        case M_STATE_SHOOT_3:
            if (info.ahead) {
                head = info.angle;
            }
            if (creature->flags == 0) {
                Creature_Shoot(
                    item, &info, &m_Cultist1Gun, head, M_GetShootDamage(item));
                creature->flags = 1;
            }
            break;

        case M_STATE_SHOOT_2:
            if (info.ahead) {
                head = info.angle;
            }
            if (item->required_anim_state == M_STATE_EMPTY) {
                if (!Creature_Shoot(
                        item, &info, &m_Cultist1Gun, head,
                        M_GetShootDamage(item))) {
                    item->goal_anim_state = M_STATE_RUN;
                }
                item->required_anim_state = M_STATE_SHOOT_2;
            }
            break;

        default:
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 0)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_DAMAGE, "Damage dealt by the cultist's shot."));
}

static void M_Setup1(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    M_SetupCommon(obj);
}

static void M_Setup1A(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    const OBJECT *const ref_obj = Object_Get(O_CULT_1);
    if (ref_obj->loaded) {
        obj->frame_base = ref_obj->frame_base;
        obj->anim_idx = ref_obj->anim_idx;
    }

    M_SetupCommon(obj);
}

static void M_Setup1B(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    const OBJECT *const ref_obj = Object_Get(O_CULT_1);
    if (ref_obj->loaded) {
        obj->frame_base = ref_obj->frame_base;
        obj->anim_idx = ref_obj->anim_idx;
    }

    M_SetupCommon(obj);
}

REGISTER_OBJECT(O_CULT_1, M_Setup1)
REGISTER_OBJECT(O_CULT_1A, M_Setup1A)
REGISTER_OBJECT(O_CULT_1B, M_Setup1B)
