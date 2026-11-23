#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/creatures/big_scorpion.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>

// clang-format off
#define M_HIT_POINTS     40
#define M_RADIUS         (WALL_L / 10) // = 102
#define M_PIVOT          50
#define M_DAMAGE         23
#define M_WALK_TURN      (DEG_1 * 5) // = 910
#define M_RUN_TURN       (DEG_1 * 10) // = 1820
#define M_AIM_TURN       (DEG_1 * 10) // = 1820
#define M_SHOOT_RANGE    SQUARE(WALL_L * 3) // = 9437184
#define M_WALK_RANGE     SQUARE(WALL_L * 2) // = 4194304
#define M_SHOT_DELAY     5
#define M_BLIND_TIME     100
#define M_OCB_WRESTLING  1
#define M_SCORPION_WAIT  80
#define M_GRIP_FRAME     37
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STOP           = 12,
    M_ANIM_STOP_TURN      = 17,
    M_ANIM_DEATH          = 19,
    M_ANIM_SCORPION_DEATH = 23,
    M_ANIM_WRESTLE        = 27,
    M_ANIM_BLIND          = 28,
    // clang-format on
} M_ANIM;

typedef enum {
    // clang-format off
    M_STATE_EMPTY          = 0,
    M_STATE_STOP           = 1,
    M_STATE_WALK           = 2,
    M_STATE_RUN            = 3,
    M_STATE_WAIT           = 4,
    M_STATE_SHOOT          = 5,
    M_STATE_WALK_SHOOT     = 6,
    M_STATE_DEATH          = 7,
    M_STATE_AIM            = 8,
    M_STATE_WALK_AIM       = 9,
    M_STATE_AIM_BURST      = 10,
    M_STATE_BURST          = 11,
    M_STATE_SCORPION_DEATH = 15,
    M_STATE_WRESTLE        = 16,
    M_STATE_BLIND          = 17,
    // clang-format on
} M_STATE;

typedef struct {
    int32_t damage;
    int16_t scorpion_wait;
} M_PRIV;

static const CREATURE_GUN m_Gun = {
    .muzzle = { .pos = { 0, 300, 64 }, .mesh_num = 7 },
};

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "scorpion_wait", &p->scorpion_wait));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "scorpion_wait", p->scorpion_wait);
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    if (M_GetOCB(item) == M_OCB_WRESTLING) {
        Item_SwitchToAnim(item, M_ANIM_WRESTLE, 0);
        item->current_anim_state = M_STATE_WRESTLE;
        item->goal_anim_state = M_STATE_WRESTLE;
    } else {
        Item_SwitchToAnim(item, M_ANIM_STOP, 0);
        item->current_anim_state = M_STATE_STOP;
        item->goal_anim_state = M_STATE_STOP;
    }
}

static void M_ChooseEnemy(const int16_t item_num, CREATURE *const soldier)
{
    const ITEM *const item = Item_Get(item_num);
    soldier->enemy = nullptr;
    int32_t best_dist = INT32_MAX;
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const baddie = LOT_GetBaddieSlot(i);
        if (baddie->item_num == NO_ITEM || baddie->item_num == item_num) {
            continue;
        }

        ITEM *const candidate = Item_Get(baddie->item_num);
        if (candidate->object_id == O_SOLDIER_1) {
            continue;
        }

        const int32_t dx = candidate->pos.x - item->pos.x;
        const int32_t dz = candidate->pos.z - item->pos.z;
        const int32_t dist = SQUARE(dx) + SQUARE(dz);
        if (dist < best_dist) {
            soldier->enemy = candidate;
            best_dist = dist;
        }
    }
}

static bool M_IsFollowDone(
    const ITEM *const item, const CREATURE *const soldier,
    const int32_t lara_dist)
{
    return (item->ai_bits & AI_FOLLOW) != 0
        && (soldier->reached_goal || lara_dist > M_WALK_RANGE);
}

static bool M_DieToScorpion(ITEM *const item)
{
    const CREATURE *const soldier = item->creature_data;
    M_PRIV *const p = item->priv;
    ITEM *const scorpion = soldier->enemy;
    if (scorpion == nullptr || scorpion->object_id != O_BIG_SCORPION
        || p->scorpion_wait >= M_SCORPION_WAIT) {
        return false;
    }

    if (!BigScorpion_IsKilling(scorpion)) {
        p->scorpion_wait++;
        return true;
    }

    const int16_t frame =
        item->current_anim_state == M_STATE_WRESTLE ? M_GRIP_FRAME : 0;
    Item_SwitchToAnim(item, M_ANIM_SCORPION_DEATH, frame);
    item->current_anim_state = M_STATE_SCORPION_DEATH;
    item->goal_anim_state = M_STATE_SCORPION_DEATH;
    item->pos = scorpion->pos;
    item->rot = scorpion->rot;
    BigScorpion_EndWrestle(scorpion);
    return true;
}

static void M_Die(ITEM *const item)
{
    if (item->current_anim_state == M_STATE_DEATH
        || item->current_anim_state == M_STATE_SCORPION_DEATH) {
        return;
    }

    if (M_DieToScorpion(item)) {
        return;
    }

    Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
    item->current_anim_state = M_STATE_DEATH;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const soldier = item->creature_data;
    const M_PRIV *const p = item->priv;
    int16_t angle = 0;
    int16_t tilt = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;
    int16_t head = 0;

    if (item->hit_points <= 0) {
        M_Die(item);
        goto finish;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (item->ai_bits != 0) {
        Creature_GetAITarget(soldier);
    } else {
        M_ChooseEnemy(item_num, soldier);
    }
    if (Creature_IsHostile(item)
        && item->current_anim_state != M_STATE_WRESTLE) {
        soldier->enemy = (ITEM *)lara_item;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    int32_t lara_dist;
    int16_t lara_angle;
    if (soldier->enemy == lara_item) {
        lara_dist = info.distance;
        lara_angle = info.angle;
    } else {
        const int32_t dx = lara_item->pos.x - item->pos.x;
        const int32_t dz = lara_item->pos.z - item->pos.z;
        lara_angle = (int16_t)(Math_Atan(dz, dx) - item->rot.y);
        lara_dist = SQUARE(dx) + SQUARE(dz);
    }

    if (!Creature_IsHostile(item) && soldier->enemy == lara_item) {
        soldier->enemy = nullptr;
    }

    Creature_Mood(item, &info, false);
    angle = Creature_Turn(item, soldier->maximum_turn);

    if (item->hit_status) {
        Creature_AlertAllGuards(item_num);
    }

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        head = lara_angle;
        soldier->flags = 0;
        soldier->maximum_turn = 0;

        if (Item_TestAnimEqual(item, M_ANIM_STOP_TURN)) {
            Creature_TurnTo(item, info.angle, M_AIM_TURN);
        }

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(soldier);
            if ((Random_GetControl() & 0xFF) == 0) {
                item->goal_anim_state = M_STATE_WAIT;
            }
        } else if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
            head = 0;
        } else if (soldier->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (Creature_CanTargetEnemy(item, &info)) {
            if (info.distance >= M_SHOOT_RANGE
                && info.zone_num == info.enemy_zone_num) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (Random_GetControl() < 0x4000) {
                item->goal_anim_state = M_STATE_AIM;
            } else {
                item->goal_anim_state = M_STATE_AIM_BURST;
            }
        } else if (
            (soldier->alerted || soldier->mood != MOOD_BORED)
            && ((item->ai_bits & AI_FOLLOW) == 0
                || (!soldier->reached_goal && lara_dist <= M_WALK_RANGE))) {
            if (soldier->mood == MOOD_BORED || info.distance <= M_WALK_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_WAIT:
        head = lara_angle;
        soldier->flags = 0;
        soldier->maximum_turn = 0;

        if ((item->ai_bits & AI_GUARD) != 0) {
            head = Creature_AIGuard(soldier);
            if ((Random_GetControl() & 0xFF) == 0) {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (Creature_CanTargetEnemy(item, &info)) {
            item->goal_anim_state = M_STATE_SHOOT;
        } else if (soldier->mood != MOOD_BORED || !info.ahead) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_WALK:
        head = lara_angle;
        soldier->flags = 0;
        soldier->maximum_turn = M_WALK_TURN;

        if ((item->ai_bits & AI_PATROL_1) != 0) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (soldier->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_RUN;
        } else if (
            (item->ai_bits & AI_GUARD) != 0
            || M_IsFollowDone(item, soldier, lara_dist)) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (Creature_CanTargetEnemy(item, &info)) {
            if (info.distance < M_SHOOT_RANGE
                || info.zone_num != info.enemy_zone_num) {
                item->goal_anim_state = M_STATE_STOP;
            } else {
                item->goal_anim_state = M_STATE_WALK_AIM;
            }
        } else if (soldier->mood == MOOD_BORED && info.ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            soldier->mood != MOOD_BORED && info.distance > M_WALK_RANGE) {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_RUN:
        if (info.ahead) {
            head = info.angle;
        }
        soldier->maximum_turn = M_RUN_TURN;
        tilt = angle / 2;

        if ((item->ai_bits & AI_GUARD) != 0
            || M_IsFollowDone(item, soldier, lara_dist)) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (soldier->mood != MOOD_ESCAPE) {
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (
                soldier->mood == MOOD_BORED
                || (soldier->mood == MOOD_STALK
                    && (item->ai_bits & AI_FOLLOW) == 0
                    && info.distance < M_WALK_RANGE)) {
                item->goal_anim_state = M_STATE_WALK;
            }
        }
        break;

    case M_STATE_WRESTLE:
        soldier->maximum_turn = 0;
        break;

    case M_STATE_AIM:
    case M_STATE_AIM_BURST:
        soldier->flags = 0;
        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;
            if (!Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (item->current_anim_state == M_STATE_AIM) {
                item->goal_anim_state = M_STATE_SHOOT;
            } else {
                item->goal_anim_state = M_STATE_BURST;
            }
        }
        break;

    case M_STATE_WALK_AIM:
        soldier->flags = 0;
        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_WALK_SHOOT;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
        }
        break;

    case M_STATE_BURST:
    case M_STATE_SHOOT:
    case M_STATE_WALK_SHOOT:
        if (item->current_anim_state == M_STATE_BURST
            && item->goal_anim_state != M_STATE_STOP
            && (soldier->mood == MOOD_ESCAPE || info.distance > M_SHOOT_RANGE
                || !Creature_CanTargetEnemy(item, &info))) {
            item->goal_anim_state = M_STATE_STOP;
        }

        if (info.ahead) {
            torso_y = info.angle;
            torso_x = info.x_angle;
        }

        if (soldier->flags == 0) {
            Creature_Shoot(item, &info, &m_Gun, torso_y, p->damage);
            soldier->flags = M_SHOT_DELAY;
        } else {
            soldier->flags--;
        }
        break;

    case M_STATE_BLIND:
        if (Lara_GetLaraInfo()->blind_timer == 0
            && (Random_GetControl() & 0x7F) == 0) {
            item->goal_anim_state = M_STATE_WAIT;
        }
        break;

    default:
        break;
    }

    if (Lara_GetLaraInfo()->blind_timer > M_BLIND_TIME
        && item->current_anim_state != M_STATE_BLIND
        && item->current_anim_state != M_STATE_WRESTLE) {
        soldier->maximum_turn = 0;
        Item_SwitchToAnim(item, M_ANIM_BLIND, Random_GetControl() & 7);
        item->current_anim_state = M_STATE_BLIND;
    }

finish:
    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);
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

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;
    obj->pivot_length = M_PIVOT;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 0)->rot.x = true;
    Object_GetBone(obj, 0)->rot.y = true;
    Object_GetBone(obj, 7)->rot.x = true;
    Object_GetBone(obj, 7)->rot.y = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by each shot that hits."));
}

REGISTER_OBJECT(O_SOLDIER_1, M_Setup)
