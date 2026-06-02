#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS 40
#define M_DAMAGE     100
#define M_RADIUS     (WALL_L / 3) // = 341
#define M_TURN       (DEG_1 * 4) // = 728
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK_1,
    M_STATE_WALK_2,
    M_STATE_ATTACK_1,
    M_STATE_ATTACK_2,
    M_STATE_ATTACK_3,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 2,
} M_ANIM;

static const BITE m_SpiderBite = {
    .pos = { .x = 0, .y = 0, .z = 41 },
    .mesh_num = 1,
};

static int32_t M_GetDamage(const ITEM *const item)
{
    OBJECT_PROPERTY_VALUE damage = {};
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

    int16_t tilt = 0;
    int16_t angle = 0;

    if (item->hit_points > 0) {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, M_TURN);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            creature->flags = 0;
            if (creature->mood == MOOD_BORED) {
                break;
            } else if (info.ahead && item->touch_bits != 0) {
                item->goal_anim_state = M_STATE_ATTACK_1;
            } else if (creature->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_WALK_1;
            } else if (
                creature->mood == MOOD_ESCAPE
                || creature->mood == MOOD_ATTACK) {
                item->goal_anim_state = M_STATE_WALK_2;
            }
            break;

        case M_STATE_WALK_1:
            if (creature->mood == MOOD_BORED) {
                break;
            } else if (info.ahead && item->touch_bits != 0) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                creature->mood == MOOD_ESCAPE
                || creature->mood == MOOD_ATTACK) {
                item->goal_anim_state = M_STATE_WALK_2;
            }
            break;

        case M_STATE_WALK_2:
            creature->flags = 0;
            if (info.ahead && item->touch_bits != 0) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                creature->mood == MOOD_BORED || creature->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_WALK_1;
            }
            break;

        case M_STATE_ATTACK_1:
            if (!creature->flags && item->touch_bits != 0) {
                Lara_TakeDamage(M_GetDamage(item), true);
                Creature_Effect(item, &m_SpiderBite, Spawn_Blood);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }
    } else if (item->current_anim_state != M_STATE_DEATH) {
        Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
        item->current_anim_state = M_STATE_DEATH;
    }

    Creature_Animate(item_num, angle, tilt);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "damage", M_DAMAGE, "Damage dealt by the big spider bite."));
}

REGISTER_OBJECT(O_BIG_SPIDER, M_Setup)
