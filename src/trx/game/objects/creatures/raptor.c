#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_ATTACK_RANGE  SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_BITE_DAMAGE   100
#define M_CHARGE_DAMAGE 100
#define M_CLOSE_RANGE   SQUARE(680) // = 462400
#define M_LUNGE_DAMAGE  100
#define M_LUNGE_RANGE   SQUARE(WALL_L * 3 / 2) // = 2359296
#define M_ROAR_CHANCE   256
#define M_RUN_TURN      (4 * DEG_1) // = 728
#define M_TOUCH         0xFF7C00
#define M_WALK_TURN     (1 * DEG_1) // = 182
#define M_HITPOINTS     20
#define M_RADIUS        (WALL_L / 3) // = 341
#define M_PIVOT_LENGTH  400
#define M_SMARTNESS     0x4000
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
    M_STATE_ATTACK_3,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 9,
} M_ANIM;

static BITE m_RaptorBite = {
    .pos = { 0, 66, 318 },
    .mesh_num = 22,
};

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const raptor = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            item->current_anim_state = M_STATE_DEATH;
            Item_SwitchToAnim(
                item, M_ANIM_DEATH + (Random_GetControl() / 16200), 0);
        }
        goto finish;
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);

    if (info.ahead) {
        head = info.angle;
    }

    Creature_Mood(item, &info, true);

    angle = Creature_Turn(item, raptor->maximum_turn);

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        if (item->required_anim_state != M_STATE_EMPTY) {
            item->goal_anim_state = item->required_anim_state;
        } else if ((item->touch_bits & M_TOUCH) != 0) {
            item->goal_anim_state = M_STATE_ATTACK_3;
        } else if (info.bite && info.distance < M_CLOSE_RANGE) {
            item->goal_anim_state = M_STATE_ATTACK_3;
        } else if (info.bite && info.distance < M_LUNGE_RANGE) {
            item->goal_anim_state = M_STATE_ATTACK_1;
        } else if (raptor->mood == MOOD_BORED) {
            item->goal_anim_state = M_STATE_WALK;
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_WALK:
        raptor->maximum_turn = M_WALK_TURN;
        if (raptor->mood != MOOD_BORED) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.ahead && Random_GetControl() < M_ROAR_CHANCE) {
            item->required_anim_state = M_STATE_WARNING;
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_RUN:
        tilt = angle;
        raptor->maximum_turn = M_RUN_TURN;
        if ((item->touch_bits & M_TOUCH) != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.bite && info.distance < M_ATTACK_RANGE) {
            if (item->goal_anim_state == M_STATE_RUN) {
                if (Random_GetControl() < 0x2000) {
                    item->goal_anim_state = M_STATE_STOP;
                } else {
                    item->goal_anim_state = M_STATE_ATTACK_2;
                }
            }
        } else if (
            info.ahead && raptor->mood != MOOD_ESCAPE
            && Random_GetControl() < M_ROAR_CHANCE) {
            item->required_anim_state = M_STATE_WARNING;
            item->goal_anim_state = M_STATE_STOP;
        } else if (raptor->mood == MOOD_BORED) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_ATTACK_1:
        tilt = angle;
        if (item->required_anim_state == M_STATE_EMPTY && info.ahead
            && (item->touch_bits & M_TOUCH) != 0) {
            Creature_Effect(item, &m_RaptorBite, Spawn_Blood);
            Lara_TakeDamage(M_LUNGE_DAMAGE, true);
            item->required_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_ATTACK_2:
        tilt = angle;
        if (item->required_anim_state == M_STATE_EMPTY && info.ahead
            && (item->touch_bits & M_TOUCH) != 0) {
            Creature_Effect(item, &m_RaptorBite, Spawn_Blood);
            Lara_TakeDamage(M_CHARGE_DAMAGE, true);
            item->required_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_ATTACK_3:
        tilt = angle;
        if (item->required_anim_state == M_STATE_EMPTY
            && (item->touch_bits & M_TOUCH) != 0) {
            Creature_Effect(item, &m_RaptorBite, Spawn_Blood);
            Lara_TakeDamage(M_BITE_DAMAGE, true);
            item->required_anim_state = M_STATE_STOP;
        }
        break;
    }
finish:
    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, tilt);
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
    obj->hit_points = M_HITPOINTS;
    obj->pivot_length = M_PIVOT_LENGTH;
    obj->radius = M_RADIUS;
    obj->smartness = M_SMARTNESS;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 21)->rot.y = true;
}

REGISTER_OBJECT(O_RAPTOR, M_Setup)
