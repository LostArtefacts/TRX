#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_TOUCH_BITS    0b00000001'10110000'00000001'00000000
#define M_RADIUS        (STEP_L / 2) // = 128
#define M_PIVOT         20
#define M_HIT_POINTS    8
#define M_DAMAGE        20
#define M_POISON_AMOUNT 512
#define M_ATTACK_DIST   SQUARE(WALL_L / 3) // = 116281
#define M_WALK_TURN     (DEG_1 * 6) // = 1092
#define M_RUN_TURN      (DEG_1 * 8) // = 1456
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STOP  = 2,
    M_ANIM_DEATH = 5,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_PINCE,
    M_STATE_STING,
    M_STATE_DEATH_1,
    M_STATE_DEATH_2,
} M_STATE;

typedef struct {
    int32_t pince_damage;
    int32_t sting_damage;
    bool is_venomous;
} M_PRIV;

static const BITE m_Pincer = {
    .pos = {},
    .mesh_num = 23,
};

static const BITE m_Sting = {
    .pos = {},
    .mesh_num = 8,
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
    CREATURE *const creature = item->creature_data;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH_1
            && item->current_anim_state != M_STATE_DEATH_2) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH_1;
        }
        goto finish;
    }

    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else {
        creature->enemy = Lara_GetItem();
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, true);
    angle = Creature_Turn(item, creature->maximum_turn);

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->maximum_turn = 0;
        creature->flags = 0;

        if (info.distance > M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (info.bite) {
            creature->maximum_turn = M_WALK_TURN;
            if ((Random_GetControl() & 1) != 0
                || (creature->enemy != nullptr
                    && creature->enemy != Lara_GetItem()
                    && creature->enemy->hit_points <= 2)) {
                item->goal_anim_state = M_STATE_PINCE;
            } else {
                item->goal_anim_state = M_STATE_STING;
            }
        } else if (!info.ahead) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WALK:
        creature->maximum_turn = M_WALK_TURN;
        if (info.distance >= M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_RUN;
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_RUN:
        creature->maximum_turn = M_RUN_TURN;
        if (info.distance < M_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_PINCE:
    case M_STATE_STING:
        creature->maximum_turn = 0;
        if (ABS(info.angle) < M_WALK_TURN) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= M_WALK_TURN;
        } else {
            item->rot.y += M_WALK_TURN;
        }

        if (creature->flags != 0 || (item->touch_bits & M_TOUCH_BITS) == 0
            || !Item_TestFrameRange(item, 21, 31)) {
            break;
        }

        creature->flags = 1;
        const M_PRIV *const p = item->priv;
        const BITE *bite = &m_Pincer;
        int32_t damage = p->pince_damage;

        if (item->current_anim_state == M_STATE_STING) {
            bite = &m_Sting;
            damage = p->sting_damage;
            if (p->is_venomous) {
                Lara_GetLaraInfo()->poison.target += M_POISON_AMOUNT;
            }
        }

        Lara_TakeDamage(damage, true);
        Creature_EffectEx(item, bite, 3, item->rot.y + DEG_180, Spawn_Blood);
        break;
    }

finish:
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;
    obj->pivot_length = M_PIVOT;

    obj->priv_size = sizeof(M_PRIV);
    obj->intelligent = true;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, pince_damage, M_DAMAGE, "Damage dealt by the pincer."),
        OBJECT_PROPERTY(
            M_PRIV, sting_damage, M_DAMAGE, "Damage dealt by the sting."),
        OBJECT_PROPERTY(
            M_PRIV, is_venomous, true,
            "Whether or not the sting poisons Lara."));
}

REGISTER_OBJECT(O_SMALL_SCORPION, M_Setup)
