#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_LION_HIT_POINTS    30
#define M_LIONESS_HIT_POINTS 25
#define M_PUMA_HIT_POINTS    45
#define M_BITE_DAMAGE        250
#define M_POUNCE_DAMAGE      150
#define M_TOUCH              0x380066
#define M_RADIUS             (WALL_L / 3) // = 341
#define M_WALK_TURN          (2 * DEG_1) // = 364
#define M_RUN_TURN           (5 * DEG_1) // = 910
#define M_ROAR_CHANCE        128
#define M_POUNCE_RANGE       SQUARE(WALL_L) // = 1048576
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_ATTACK_1,
    M_STATE_DEATH,
    M_STATE_WARNING,
    M_STATE_ATTACK_2,
} M_STATE;

typedef enum {
    M_LION_ANIM_DEATH = 7,
} M_LION_ANIM;

typedef enum {
    M_PUMA_ANIM_DEATH = 4,
} M_PUMA_ANIM;

typedef struct {
    int32_t pounce_damage;
    int32_t bite_damage;
} M_PRIV;

static BITE m_LionBite = {
    .pos = { -2, -10, 132 },
    .mesh_num = 21,
};

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const lion = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            item->current_anim_state = M_STATE_DEATH;
            int16_t anim_idx = item->object_id == O_PUMA ? M_PUMA_ANIM_DEATH
                                                         : M_LION_ANIM_DEATH;
            Item_SwitchToAnim(
                item, anim_idx + (int16_t)(Random_GetControl() / 0x4000), 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, lion->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (lion->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (info.ahead && (item->touch_bits & M_TOUCH)) {
                item->goal_anim_state = M_STATE_ATTACK_2;
            } else if (info.ahead && info.distance < M_POUNCE_RANGE) {
                item->goal_anim_state = M_STATE_ATTACK_1;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_WALK:
            lion->maximum_turn = M_WALK_TURN;
            if (lion->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Random_GetControl() < M_ROAR_CHANCE) {
                item->required_anim_state = M_STATE_WARNING;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_RUN:
            lion->maximum_turn = M_RUN_TURN;
            tilt = angle;
            if (lion->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.ahead && info.distance < M_POUNCE_RANGE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if ((item->touch_bits & M_TOUCH) && info.ahead) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                lion->mood != MOOD_ESCAPE
                && Random_GetControl() < M_ROAR_CHANCE) {
                item->required_anim_state = M_STATE_WARNING;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_ATTACK_1:
            if (item->required_anim_state == M_STATE_EMPTY
                && (item->touch_bits & M_TOUCH)) {
                Lara_TakeDamage(p->pounce_damage, true);
                item->required_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_ATTACK_2:
            if (item->required_anim_state == M_STATE_EMPTY
                && (item->touch_bits & M_TOUCH)) {
                Creature_Effect(item, &m_LionBite, Spawn_Blood);
                Lara_TakeDamage(p->bite_damage, true);
                item->required_anim_state = M_STATE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->lot_setup = LOT_Setup(LOT_SETUP_QUADRUPED);

    obj->radius = M_RADIUS;
    obj->pivot_length = 400;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 19)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, pounce_damage, M_POUNCE_DAMAGE,
            "Damage dealt by the pounce attack."),
        OBJECT_PROPERTY(
            M_PRIV, bite_damage, M_BITE_DAMAGE,
            "Damage dealt by the bite attack."));
}

static void M_SetupLion(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);

    obj->smartness = 0x7FFF;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_LION_HIT_POINTS, "Maximum hit points."));
}

static void M_SetupLioness(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);

    obj->smartness = 0x2000;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_LIONESS_HIT_POINTS, "Maximum hit points."));
}

static void M_SetupPuma(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);

    obj->smartness = 0x2000;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_PUMA_HIT_POINTS, "Maximum hit points."));
}

REGISTER_OBJECT(O_LION, M_SetupLion)
REGISTER_OBJECT(O_LIONESS, M_SetupLioness)
REGISTER_OBJECT(O_PUMA, M_SetupPuma)
