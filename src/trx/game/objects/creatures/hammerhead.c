#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS   45
#define M_DAMAGE       120
#define M_RADIUS       (WALL_L / 3) // = 341
#define M_PIVOT        300
#define M_TOUCH_BITS   0b00110100'00000000 // = 0x3400
#define M_TURN         (DEG_1 * 7) // = 1274
#define M_SWIM_RANGE   SQUARE(WALL_L) // = 1048576
#define M_BITE_RANGE   SQUARE(WALL_L * 2 / 3) // = 465124
// clang-format on

typedef enum {
    // clang-format off
    M_STATE_STOP      = 0,
    M_STATE_SWIM_SLOW = 1,
    M_STATE_SWIM_FAST = 2,
    M_STATE_BITE      = 3,
    M_STATE_DEATH     = 5,
    M_STATE_KILL      = 6,
    // clang-format on
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_DEATH = 4,
    M_ANIM_STOP  = 8,
    // clang-format on
} M_ANIM;

typedef struct {
    int32_t damage;
} M_PRIV;

static const BITE m_Bite = {
    .pos = { .x = 0, .y = 0, .z = 0 },
    .mesh_num = 12,
};

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_STOP, 0);
    item->current_anim_state = M_STATE_STOP;
    item->goal_anim_state = M_STATE_STOP;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        }
        Creature_Float(item_num);
        return;
    }

    CREATURE *const shark = item->creature_data;
    const M_PRIV *const p = item->priv;

    if (item->ai_bits != 0) {
        Creature_GetAITarget(shark);
    } else if (shark->hurt_by_lara) {
        shark->enemy = Lara_GetItem();
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, true);
    const int16_t angle = Creature_Turn(item, shark->maximum_turn);
    const int16_t joint = angle * 2;

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        item->goal_anim_state = M_STATE_SWIM_SLOW;
        shark->flags = 0;
        break;

    case M_STATE_SWIM_SLOW:
        shark->maximum_turn = M_TURN;
        if (info.distance > M_SWIM_RANGE) {
            item->goal_anim_state = M_STATE_SWIM_FAST;
        } else if (info.distance < M_BITE_RANGE) {
            item->goal_anim_state = M_STATE_BITE;
        }
        break;

    case M_STATE_SWIM_FAST:
        if (info.distance < M_SWIM_RANGE) {
            item->goal_anim_state = M_STATE_SWIM_SLOW;
        }
        break;

    case M_STATE_BITE:
        if (shark->flags == 0 && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Lara_TakeDamage(p->damage, true);
            Creature_Effect(item, &m_Bite, Spawn_Blood);
            shark->flags = 1;
        }
        break;

    default:
        break;
    }

    Creature_Tilt(item, 0);
    Creature_Joint(item, 0, -joint);
    Creature_Joint(item, 1, -joint);
    Creature_Joint(item, 2, -joint);
    Creature_Joint(item, 3, joint);

    if (item->current_anim_state == M_STATE_KILL) {
        Item_Animate(item);
    } else {
        Creature_Animate(item_num, angle, 0);
        Creature_Underwater(item, M_RADIUS);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);
    obj->lot_setup.fly = STEP_L / 8;

    obj->priv_size = sizeof(M_PRIV);
    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 0)->rot.y = true;
    Object_GetBone(obj, 1)->rot.y = true;
    Object_GetBone(obj, 2)->rot.y = true;
    Object_GetBone(obj, 9)->rot.y = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by bite attacks."));
}

REGISTER_OBJECT(O_HAMMERHEAD, M_Setup)
