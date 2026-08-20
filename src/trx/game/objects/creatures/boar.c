#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_PIVOT       50
#define M_RADIUS      (WALL_L / 10) // = 102
#define M_HIT_POINTS  40
#define M_DAMAGE      30
#define M_ATTACK_DIST SQUARE(WALL_L) // = 4194304
#define M_BITE_DIST   SQUARE(STEP_L) // = 65536
#define M_ATTACK_TURN (DEG_1 * 6) // = 1092
#define M_RUN_TURN    (DEG_1 * 3) // = 546
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_DEATH = 5,
    M_ANIM_STOP  = 6,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_ATTACK,
    M_STATE_CHARGE,
    M_STATE_WARN,
    M_STATE_DEATH,
} M_STATE;

typedef struct {
    int32_t damage;
} M_PRIV;

static const BITE m_Bite = {
    .pos = {},
    .mesh_num = 14,
};

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_STOP, 0);
    item->current_anim_state = M_STATE_STOP;
    item->goal_anim_state = M_STATE_STOP;
}

static void M_CalculateEnemy(ITEM *const item)
{
    CREATURE *const boar = item->creature_data;
    ITEM *const lara_item = Lara_GetItem();
    boar->enemy = lara_item;
    int32_t best_distance = XYZ_32_GetLength2((XYZ_32) {
        .x = lara_item->pos.x - item->pos.x,
        .y = 0,
        .z = lara_item->pos.z - item->pos.z,
    });

    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM) {
            continue;
        }

        ITEM *const candidate = Item_Get(creature->item_num);
        if (candidate->object_id == item->object_id) {
            continue;
        }

        const int32_t distance = XYZ_32_GetLength2((XYZ_32) {
            .x = candidate->pos.x - item->pos.x,
            .y = 0,
            .z = candidate->pos.z - item->pos.z,
        });
        if (distance < best_distance) {
            boar->enemy = candidate;
            best_distance = distance;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    int16_t neck_x = 0;
    int16_t neck_y = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_STATE_DEATH;
        }
        goto finish;
    }

    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else {
        M_CalculateEnemy(item);
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    Creature_UpdateMood(item, &info, true);
    if (creature->flags != 0) {
        creature->mood = MOOD_ESCAPE;
    }
    Creature_ApplyMood(item, &info, true);

    angle = Creature_Turn(item, creature->maximum_turn);
    if (info.ahead) {
        neck_y = info.angle >> 1;
    }

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->maximum_turn = 0;
        if ((info.ahead && info.distance != 0) || creature->flags != 0) {
            item->goal_anim_state = M_STATE_ATTACK;
        } else if ((Random_GetControl() & 0x7F) != 0) {
            neck_y = Creature_AIGuard(creature) >> 1;
        } else {
            item->goal_anim_state = M_STATE_CHARGE;
        }
        break;

    case M_STATE_ATTACK:
        if (info.distance >= M_ATTACK_DIST) {
            creature->maximum_turn = M_ATTACK_TURN;
            creature->flags = 0;
        } else {
            creature->maximum_turn = M_RUN_TURN;
            neck_x = -info.distance;
        }

        if (creature->flags == 0 && info.distance < M_BITE_DIST && info.bite) {
            if (creature->enemy == Lara_GetItem()) {
                const M_PRIV *const p = item->priv;
                Lara_TakeDamage(p->damage, true);
            }

            Creature_Effect(item, &m_Bite, Spawn_Blood);
            creature->flags = 1;
            item->goal_anim_state = M_STATE_WARN;
        }
        break;

    case M_STATE_CHARGE:
        creature->maximum_turn = 0;
        if (info.ahead && info.distance != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if ((Random_GetControl() & 0x7F) == 0) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_WARN:
        creature->maximum_turn = 0;
        break;
    }

finish:
    Creature_Joint(item, 0, neck_x);
    Creature_Joint(item, 1, neck_y);
    Creature_Joint(item, 2, neck_x);
    Creature_Joint(item, 3, neck_y);
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = Creature_Collision;
    obj->control_func = M_Control;

    obj->radius = M_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;

    obj->priv_size = sizeof(M_PRIV);
    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 12)->rot.y = true;
    Object_GetBone(obj, 12)->rot.z = true;
    Object_GetBone(obj, 13)->rot.y = true;
    Object_GetBone(obj, 13)->rot.z = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the boar's attack."));
}

REGISTER_OBJECT(O_BOAR, M_Setup)
