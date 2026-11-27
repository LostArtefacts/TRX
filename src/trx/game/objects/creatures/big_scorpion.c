#include <trx/game/objects/creatures/big_scorpion.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/cutseq.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_TOUCH_BITS     0b00000001'10110000'00000001'00000000 // = 0x1B00100
#define M_HIT_POINTS     80
#define M_RADIUS         (WALL_L / 2) // = 512
#define M_PIVOT          50
#define M_DAMAGE         120
#define M_ENEMY_DAMAGE   15
#define M_POISON_AMOUNT  2048
#define M_ATTACK_RANGE   SQUARE(WALL_L * 4 / 3) // = 1863225
#define M_RUN_RANGE      SQUARE(WALL_L * 5 / 6) // = 727609
#define M_WALK_TURN      (DEG_1 * 2) // = 364
#define M_RUN_TURN       (DEG_1 * 3) // = 546
#define M_PROBE_DIST     (WALL_L * 2 / 3) // = 682
#define M_PROBE_STEP     (STEP_L * 2) // = 512
#define M_TILT_STEP      STEP_L
#define M_OCB_WRESTLING  1
#define M_WRESTLE_ROUNDS 6
#define M_WRESTLE_END    99
#define M_CUTSCENE_NUM   4
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STOP    = 2,
    M_ANIM_DEATH   = 5,
    M_ANIM_KILL    = 6,
    M_ANIM_WRESTLE = 7,
    // clang-format on
} M_ANIM;

typedef enum {
    // clang-format off
    M_STATE_EMPTY   = 0,
    M_STATE_STOP    = 1,
    M_STATE_WALK    = 2,
    M_STATE_RUN     = 3,
    M_STATE_PINCE   = 4,
    M_STATE_STING   = 5,
    M_STATE_DEATH   = 6,
    M_STATE_KILL    = 7,
    M_STATE_WRESTLE = 8,
    // clang-format on
} M_STATE;

typedef struct {
    int32_t pince_damage;
    int32_t sting_damage;
    bool is_venomous;
    int32_t wrestle_round;
    bool killed_lara;
} M_PRIV;

static const BITE m_Sting = {
    .pos = {},
    .mesh_num = 8,
};

static const BITE m_Pincer = {
    .pos = {},
    .mesh_num = 23,
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
    MUST(JSON_READ_OPT(io, "wrestle_round", &p->wrestle_round));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "wrestle_round", p->wrestle_round);
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->wrestle_round = M_GetOCB(item);
    if (p->wrestle_round == M_OCB_WRESTLING) {
        Item_SwitchToAnim(item, M_ANIM_WRESTLE, 0);
        item->current_anim_state = M_STATE_WRESTLE;
        item->goal_anim_state = M_STATE_WRESTLE;
    } else {
        Item_SwitchToAnim(item, M_ANIM_STOP, 0);
        item->current_anim_state = M_STATE_STOP;
        item->goal_anim_state = M_STATE_STOP;
    }
}

static int32_t M_GetFloor(
    const ITEM *const item, const XYZ_32 pos, int16_t *const room_num)
{
    const SECTOR *const sector = Room_GetSector(pos, room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    return ABS(item->pos.y - height) > M_PROBE_STEP ? item->pos.y : height;
}

static XZ_16 M_GetSlope(const ITEM *const item)
{
    const int32_t s = (M_PROBE_DIST * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    const int32_t c = (M_PROBE_DIST * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;

    int16_t room_num = item->room_num;
    const int32_t front =
        M_GetFloor(item, (XYZ_32) { x + s, y, z + c }, &room_num);
    const int32_t back =
        M_GetFloor(item, (XYZ_32) { x - s, y, z - c }, &room_num);
    const int32_t left =
        M_GetFloor(item, (XYZ_32) { x - c, y, z + s }, &room_num);
    const int32_t right =
        M_GetFloor(item, (XYZ_32) { x + c, y, z - s }, &room_num);

    return (XZ_16) {
        .x = (int16_t)Math_Atan(M_PROBE_DIST * 2, back - front),
        .z = (int16_t)Math_Atan(M_PROBE_DIST * 2, right - left),
    };
}

static void M_Lean(int16_t *const rot, const int16_t target)
{
    if (ABS(target - *rot) < M_TILT_STEP) {
        *rot = target;
    } else if (target > *rot) {
        *rot += M_TILT_STEP;
    } else if (target < *rot) {
        *rot -= M_TILT_STEP;
    }
}

static void M_ChooseEnemy(const int16_t item_num, CREATURE *const scorpion)
{
    const ITEM *const item = Item_Get(item_num);
    scorpion->enemy = nullptr;
    int32_t best_dist = INT32_MAX;
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const baddie = LOT_GetBaddieSlot(i);
        if (baddie->item_num == NO_ITEM || baddie->item_num == item_num) {
            continue;
        }

        ITEM *const candidate = Item_Get(baddie->item_num);
        if (candidate->object_id == O_BIG_SCORPION) {
            continue;
        }

        const int32_t dx = candidate->pos.x - item->pos.x;
        const int32_t dz = candidate->pos.z - item->pos.z;
        const int32_t dist = SQUARE(dx) + SQUARE(dz);
        if (dist < best_dist) {
            scorpion->enemy = candidate;
            best_dist = dist;
        }
    }
}

static bool M_IsWrestling(const M_PRIV *const p)
{
    return p->wrestle_round > 0 && p->wrestle_round <= M_WRESTLE_ROUNDS;
}

static void M_RemoveWrestler(const ITEM *const item)
{
    int16_t target_num = Room_Get(item->room_num)->item_num;
    while (target_num != NO_ITEM) {
        const ITEM *const target = Item_Get(target_num);
        if (target->object_id == O_SOLDIER_1
            && M_GetOCB(target) == M_OCB_WRESTLING) {
            LOT_DisableBaddieAI(target_num);
            Item_Destroy(target_num);
            return;
        }
        target_num = target->next_item;
    }
}

static void M_Die(ITEM *const item)
{
    CREATURE *const scorpion = item->creature_data;
    const M_PRIV *const p = item->priv;
    item->hit_points = 0;

    if (item->current_anim_state != M_STATE_DEATH && M_IsWrestling(p)) {
        Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
        item->current_anim_state = M_STATE_DEATH;
        scorpion->maximum_turn = 0;
        M_RemoveWrestler(item);
        Item_SetVisible(item, false);
        CutSeq_Request(M_CUTSCENE_NUM, true);
    } else if (
        item->current_anim_state != M_STATE_DEATH
        && item->current_anim_state != M_STATE_KILL) {
        Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
        item->current_anim_state = M_STATE_DEATH;
    } else if (CutSeq_IsActive()) {
        item->frame_num = Item_GetAnim(item)->frame_end - 1;
        Item_SetVisible(item, false);
    } else if (item->current_anim_state == M_STATE_DEATH && !item->is_visible) {
        Item_SetVisible(item, true);
    }
}

static bool M_Attack(
    ITEM *const item, CREATURE *const scorpion, const AI_INFO *const info)
{
    ITEM *const lara_item = Lara_GetItem();
    ITEM *const enemy = scorpion->enemy;
    M_PRIV *const p = item->priv;
    const int16_t lara_hit_points = lara_item->hit_points;

    scorpion->maximum_turn = 0;
    Creature_TurnTo(item, info->angle, M_WALK_TURN);

    if (scorpion->flags != 0) {
        return true;
    }

    if (enemy != nullptr && enemy != lara_item
        && info->distance < M_ATTACK_RANGE) {
        Item_TakeDamage(enemy, M_ENEMY_DAMAGE, IDF_NONE, item);
        if (enemy->hit_points <= 0) {
            item->goal_anim_state = M_STATE_KILL;
            scorpion->maximum_turn = 0;
        }
        scorpion->flags = 1;
        Creature_EffectEx(
            item, &m_Sting, 10, item->rot.y + DEG_180, Spawn_Blood);
        return true;
    }

    if ((item->touch_bits & M_TOUCH_BITS) == 0) {
        return true;
    }

    if (item->current_anim_state == M_STATE_STING) {
        Lara_TakeDamage(p->sting_damage, true);
        if (p->is_venomous) {
            Lara_GetLaraInfo()->poison.target += M_POISON_AMOUNT;
        }
        Creature_EffectEx(
            item, &m_Sting, 10, item->rot.y + DEG_180, Spawn_Blood);
    } else {
        Lara_TakeDamage(p->pince_damage, true);
        Creature_EffectEx(
            item, &m_Pincer, 10, item->rot.y + DEG_180, Spawn_Blood);
    }
    scorpion->flags = 1;

    if (lara_hit_points > 0 && lara_item->hit_points <= 0) {
        Creature_KillLara(
            item, M_ANIM_KILL, M_STATE_KILL, LA_DEATH_BIG_SCORPION);
        scorpion->maximum_turn = 0;
        p->killed_lara = true;
        return false;
    }
    return true;
}

static bool M_Think(
    const int16_t item_num, ITEM *const item, int16_t *const angle)
{
    CREATURE *const scorpion = item->creature_data;
    M_PRIV *const p = item->priv;

    if (item->ai_bits != 0) {
        Creature_GetAITarget(scorpion);
    } else if (
        scorpion->hurt_by_lara && item->current_anim_state != M_STATE_WRESTLE) {
        scorpion->enemy = Lara_GetItem();
    } else {
        M_ChooseEnemy(item_num, scorpion);
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);
    ITEM *const enemy = scorpion->enemy;
    Creature_Mood(item, &info, true);
    *angle = Creature_Turn(item, scorpion->maximum_turn);

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        scorpion->maximum_turn = 0;
        scorpion->flags = 0;
        if (info.distance <= M_ATTACK_RANGE && info.bite) {
            scorpion->maximum_turn = M_WALK_TURN;
            if ((Random_GetControl() & 1) != 0
                || (enemy->object_id == O_SOLDIER_1
                    && enemy->hit_points <= M_ENEMY_DAMAGE)) {
                item->goal_anim_state = M_STATE_PINCE;
            } else {
                item->goal_anim_state = M_STATE_STING;
            }
        } else if (info.distance > M_ATTACK_RANGE || !info.ahead) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_WALK:
        scorpion->maximum_turn = M_WALK_TURN;
        if (info.distance < M_ATTACK_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.distance > M_RUN_RANGE) {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_RUN:
        scorpion->maximum_turn = M_RUN_TURN;
        if (info.distance < M_ATTACK_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_WRESTLE:
        scorpion->maximum_turn = 0;
        if (Item_TestFrameEqual(item, -1)) {
            p->wrestle_round++;
        }
        if (enemy->hit_points <= 0 || p->wrestle_round > M_WRESTLE_ROUNDS) {
            item->goal_anim_state = M_STATE_KILL;
            Item_TakeFatalDamage(enemy, item);
        }
        break;

    case M_STATE_PINCE:
    case M_STATE_STING:
        return M_Attack(item, scorpion, &info);

    default:
        break;
    }
    return true;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    const XZ_16 slope = M_GetSlope(item);
    int16_t angle = 0;

    if (p->killed_lara) {
        Creature_WatchKill(item);
    }

    if (item->hit_points <= 0) {
        M_Die(item);
    } else if (!M_Think(item_num, item, &angle)) {
        return;
    }

    M_Lean(&item->rot.x, slope.x);
    M_Lean(&item->rot.z, slope.z);
    if (!CutSeq_IsActive()) {
        Creature_Animate(item_num, angle, 0);
    }
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

bool BigScorpion_IsKilling(const ITEM *const item)
{
    return Item_TestAnimEqual(item, M_ANIM_KILL);
}

void BigScorpion_EndWrestle(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    p->wrestle_round = M_WRESTLE_END;
}

REGISTER_OBJECT(O_BIG_SCORPION, M_Setup)
