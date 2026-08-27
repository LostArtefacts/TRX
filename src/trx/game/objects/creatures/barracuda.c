#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS     12
#define M_DAMAGE         100
#define M_TOUCH_BITS     0b11100000 // = 0xE0
#define M_RADIUS         (WALL_L / 5) // = 204
#define M_SWIM_1_TURN    (DEG_1 * 2) // = 364
#define M_SWIM_2_TURN    (DEG_1 * 4) // = 728
#define M_ATTACK_1_RANGE SQUARE(WALL_L * 2 / 3) // = 465124
#define M_ATTACK_2_RANGE SQUARE(WALL_L / 3) // = 116281
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_SWIM_1,
    M_STATE_SWIM_2,
    M_STATE_ATTACK_1,
    M_STATE_ATTACK_2,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 6,
} M_ANIM;

typedef struct {
    int32_t damage;
} M_PRIV;

static const BITE m_BarracudaBite = {
    .pos = { .x = 2, .y = -60, .z = 121 },
    .mesh_num = 7,
};

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;

    if (item->hit_points > 0) {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, false);

        int16_t head = 0;
        int16_t angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            creature->flags = 0;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_SWIM_1;
            } else if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_1;
            } else if (creature->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_SWIM_1;
            } else {
                item->goal_anim_state = M_STATE_SWIM_2;
            }
            break;

        case M_STATE_SWIM_1:
            creature->maximum_turn = M_SWIM_1_TURN;
            if (creature->mood == MOOD_BORED) {
            } else if (info.ahead && (item->touch_bits & M_TOUCH_BITS) != 0) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (creature->mood != MOOD_STALK) {
                item->goal_anim_state = M_STATE_SWIM_2;
            }
            break;

        case M_STATE_SWIM_2:
            creature->maximum_turn = M_SWIM_2_TURN;
            creature->flags = 0;
            if (creature->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_SWIM_1;
            } else if (info.ahead && info.distance < M_ATTACK_2_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_2;
            } else if (info.ahead && info.distance < M_ATTACK_1_RANGE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (creature->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_SWIM_1;
            }
            break;

        case M_STATE_ATTACK_1:
        case M_STATE_ATTACK_2:
            if (info.ahead) {
                head = info.angle;
            }
            if (creature->flags == 0
                && (item->touch_bits & M_TOUCH_BITS) != 0) {
                Lara_TakeDamage(p->damage, true);
                Creature_Effect(item, &m_BarracudaBite, Spawn_Blood);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }

        Creature_Head(item, head);
        Creature_Animate(item_num, angle, 0);
        Creature_Underwater(item, STEP_L);
    } else {
        if (item->current_anim_state != M_ANIM_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
            Carrier_TestItemDrops(item_num);
        }
        Creature_Float(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->priv_size = sizeof(M_PRIV);
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);

    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 6)->rot.y = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the barracuda bite."));
}

REGISTER_OBJECT(O_BARRACUDA, M_Setup)
REGISTER_OBJECT(O_FISH_MUTANT, M_Setup)
