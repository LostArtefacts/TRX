#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS    (g_TRVersion == 1 ? 6 : 10)
#define M_BITE_DAMAGE   100
#define M_POUNCE_DAMAGE 50
#define M_RADIUS        (WALL_L / 3) // = 341
#define M_WALK_TURN     (2 * DEG_1) // = 364
#define M_RUN_TURN      (5 * DEG_1) // = 910
#define M_STALK_TURN    (2 * DEG_1) // = 364
#define M_ATTACK_RANGE  SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_STALK_RANGE   SQUARE(WALL_L * 3) // = 9437184
#define M_BITE_RANGE    SQUARE(345) // = 119025
#define M_WAKE_CHANCE   32
#define M_SLEEP_CHANCE  32
#define M_HOWL_CHANCE   384
#define M_TOUCH         0x774F
#define M_SMARTNESS     0x2000
#define M_SLEEP_FRAME   96
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_JUMP,
    M_STATE_STALK,
    M_STATE_ATTACK,
    M_STATE_HOWL,
    M_STATE_SLEEP,
    M_STATE_CROUCH,
    M_STATE_FAST_TURN,
    M_STATE_DEATH,
    M_STATE_BITE,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 20,
} M_ANIM;

typedef struct {
    int32_t pounce_damage;
    int32_t bite_damage;
} M_PRIV;

static BITE m_WolfJawBite = { .pos = { 0, -14, 174 }, .mesh_num = 6 };

static void M_Initialise(const int16_t item_num)
{
    Item_Get(item_num)->frame_num = M_SLEEP_FRAME;
    Creature_Initialise(item_num);
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const wolf = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            item->current_anim_state = M_STATE_DEATH;
            Item_SwitchToAnim(
                item, M_ANIM_DEATH + (int16_t)(Random_GetControl() / 11000), 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, wolf->maximum_turn);

        switch (item->current_anim_state) {
        case M_STATE_SLEEP:
            head = 0;
            if (wolf->mood == MOOD_ESCAPE
                || info.zone_num == info.enemy_zone_num) {
                item->required_anim_state = M_STATE_CROUCH;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Random_GetControl() < M_WAKE_CHANCE) {
                item->required_anim_state = M_STATE_WALK;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        case M_STATE_WALK:
            wolf->maximum_turn = M_WALK_TURN;
            if (wolf->mood != MOOD_BORED) {
                item->goal_anim_state = M_STATE_STALK;
                item->required_anim_state = M_STATE_EMPTY;
            } else if (Random_GetControl() < M_SLEEP_CHANCE) {
                item->required_anim_state = M_STATE_SLEEP;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_CROUCH:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (wolf->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.distance < M_BITE_RANGE && info.bite) {
                item->goal_anim_state = M_STATE_BITE;
            } else if (wolf->mood == MOOD_STALK) {
                item->goal_anim_state = M_STATE_STALK;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_STALK:
            wolf->maximum_turn = M_STALK_TURN;
            if (wolf->mood == MOOD_ESCAPE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (info.distance < M_BITE_RANGE && info.bite) {
                item->goal_anim_state = M_STATE_BITE;
            } else if (info.distance > M_STALK_RANGE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (wolf->mood == MOOD_ATTACK) {
                if (!info.ahead || info.distance > M_ATTACK_RANGE
                    || (info.enemy_facing < FRONT_ARC
                        && info.enemy_facing > -FRONT_ARC)) {
                    item->goal_anim_state = M_STATE_RUN;
                }
            } else if (Random_GetControl() < M_HOWL_CHANCE) {
                item->required_anim_state = M_STATE_HOWL;
                item->goal_anim_state = M_STATE_CROUCH;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_CROUCH;
            }
            break;

        case M_STATE_RUN:
            wolf->maximum_turn = M_RUN_TURN;
            tilt = angle;
            if (info.ahead && info.distance < M_ATTACK_RANGE) {
                if (info.distance > (M_ATTACK_RANGE / 2)
                    && (info.enemy_facing > FRONT_ARC
                        || info.enemy_facing < -FRONT_ARC)) {
                    item->required_anim_state = M_STATE_STALK;
                    item->goal_anim_state = M_STATE_CROUCH;
                } else {
                    item->goal_anim_state = M_STATE_ATTACK;
                    item->required_anim_state = M_STATE_EMPTY;
                }
            } else if (
                wolf->mood == MOOD_STALK && info.distance < M_STALK_RANGE) {
                item->required_anim_state = M_STATE_STALK;
                item->goal_anim_state = M_STATE_CROUCH;
            } else if (wolf->mood == MOOD_BORED) {
                item->goal_anim_state = M_STATE_CROUCH;
            }
            break;

        case M_STATE_ATTACK:
            tilt = angle;
            if (item->required_anim_state == M_STATE_EMPTY
                && (item->touch_bits & M_TOUCH)) {
                Creature_Effect(item, &m_WolfJawBite, Spawn_Blood);
                Lara_TakeDamage(p->pounce_damage, true);
                item->required_anim_state = M_STATE_RUN;
            }
            item->goal_anim_state = M_STATE_RUN;
            break;

        case M_STATE_BITE:
            if (item->required_anim_state == M_STATE_EMPTY
                && (item->touch_bits & M_TOUCH) && info.ahead) {
                Creature_Effect(item, &m_WolfJawBite, Spawn_Blood);
                Lara_TakeDamage(p->bite_damage, true);
                item->required_anim_state = M_STATE_CROUCH;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->pivot_length = 375;
    obj->radius = M_RADIUS;
    obj->smartness = M_SMARTNESS;
    obj->lot_setup = LOT_Setup(LOT_SETUP_QUADRUPED);
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 2)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY(
            M_PRIV, pounce_damage, M_POUNCE_DAMAGE,
            "Damage dealt by the pounce attack."),
        OBJECT_PROPERTY(
            M_PRIV, bite_damage, M_BITE_DAMAGE,
            "Damage dealt by the bite attack."));
}

REGISTER_OBJECT(O_WOLF, M_Setup)
