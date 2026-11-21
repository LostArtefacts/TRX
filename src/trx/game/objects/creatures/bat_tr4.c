#include <trx/game/objects/creatures/bat_tr4.h>

#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS   5
#define M_DAMAGE       2
#define M_RADIUS       (WALL_L / 10) // = 102
#define M_PIVOT_LENGTH 10
#define M_TURN         (20 * DEG_1) // = 3640
#define M_BITE_RANGE   SQUARE(STEP_L) // = 65536
#define M_BITE_HEIGHT  896
#define M_WAKE_RANGE   SQUARE(WALL_L * 5) // = 26214400
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_FLY   = 1,
    M_ANIM_FALL  = 3,
    M_ANIM_HANG  = 5,
    // clang-format on
} M_ANIM;

typedef enum {
    // clang-format off
    M_STATE_EMPTY  = 0,
    M_STATE_START  = 1,
    M_STATE_FLY    = 2,
    M_STATE_ATTACK = 3,
    M_STATE_FALL   = 4,
    M_STATE_DEATH  = 5,
    M_STATE_HANG   = 6,
    // clang-format on
} M_STATE;

typedef struct {
    int32_t damage;
} M_PRIV;

static const BITE m_Bite = {
    .pos = { 0, 16, 45 },
    .mesh_num = 4,
};

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_HANG, 0);
    item->current_anim_state = M_STATE_HANG;
    item->goal_anim_state = M_STATE_HANG;
}

static void M_ChooseEnemy(const ITEM *const item)
{
    CREATURE *const bat = item->creature_data;
    bat->enemy = Lara_GetItem();
    int32_t best_distance = INT32_MAX;

    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM
            || creature->item_num == Item_GetIndex(item)) {
            continue;
        }

        ITEM *const candidate = Item_Get(creature->item_num);
        if (candidate->object_id != O_VON_CROY) {
            continue;
        }

        const int32_t distance = XYZ_32_GetLength2((XYZ_32) {
            .x = candidate->pos.x - item->pos.x,
            .y = 0,
            .z = candidate->pos.z - item->pos.z,
        });
        if (distance < best_distance) {
            bat->enemy = candidate;
            best_distance = distance;
        }
    }
}

static bool M_CanBite(const ITEM *const item, const AI_INFO *const info)
{
    const CREATURE *const bat = item->creature_data;
    if (bat->flags != 0) {
        return false;
    }
    if (item->touch_bits != 0) {
        return true;
    }
    return bat->enemy != Lara_GetItem() && info->distance < M_BITE_RANGE
        && info->ahead && ABS(item->pos.y - bat->enemy->pos.y) < M_BITE_HEIGHT;
}

static void M_ControlDead(ITEM *const item)
{
    if (item->current_anim_state == M_STATE_ATTACK) {
        Item_SwitchToAnim(item, M_ANIM_FLY, 0);
        item->current_anim_state = M_STATE_FLY;
        item->goal_anim_state = M_STATE_FLY;
    } else if (item->pos.y < item->floor) {
        item->gravity = true;
        Item_SwitchToAnim(item, M_ANIM_FALL, 0);
        item->current_anim_state = M_STATE_FALL;
        item->goal_anim_state = M_STATE_FALL;
        item->speed = 0;
    } else {
        item->goal_anim_state = M_STATE_DEATH;
        item->gravity = false;
        item->pos.y = item->floor;
    }
}

static int16_t M_ControlAlive(ITEM *const item)
{
    CREATURE *const bat = item->creature_data;
    const M_PRIV *const p = item->priv;

    if (item->ai_bits != 0) {
        Creature_GetAITarget(bat);
    } else {
        M_ChooseEnemy(item);
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);
    Creature_UpdateMood(item, &info, false);
    if (bat->flags != 0) {
        bat->mood = MOOD_ESCAPE;
    }
    Creature_ApplyMood(item, &info, false);
    const int16_t angle = Creature_Turn(item, M_TURN);

    switch (item->current_anim_state) {
    case M_STATE_FLY:
        if (info.distance < M_BITE_RANGE || (Random_GetControl() & 0x3F) == 0) {
            bat->flags = 0;
        }
        if (M_CanBite(item, &info)) {
            item->goal_anim_state = M_STATE_ATTACK;
        }
        break;

    case M_STATE_ATTACK:
        if (M_CanBite(item, &info)) {
            Creature_Effect(item, &m_Bite, Spawn_Blood);
            if (bat->enemy == Lara_GetItem()) {
                Lara_TakeDamage(p->damage, true);
            }
            bat->flags = 1;
        } else {
            item->goal_anim_state = M_STATE_FLY;
            bat->mood = MOOD_BORED;
        }
        break;

    case M_STATE_HANG:
        if (info.distance < M_WAKE_RANGE || item->hit_status
            || bat->hurt_by_lara) {
            item->goal_anim_state = M_STATE_START;
        }
        break;
    }

    return angle;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    int16_t angle = 0;
    if (item->hit_points <= 0) {
        M_ControlDead(item);
    } else {
        angle = M_ControlAlive(item);
    }

    Creature_Animate(item_num, angle, 0);
}

void Bat_TR4_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT_LENGTH;
    obj->radius = M_RADIUS;
    obj->lot_setup = LOT_Setup(LOT_SETUP_FLYER);
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the bat attack."));
}
