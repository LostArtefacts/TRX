#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/random.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_TOUCH_BITS        0b01001000'00000000
#define M_RADIUS            (WALL_L / 6) // = 170
#define M_PIVOT             50
#define M_HIT_POINTS        15
#define M_DAMAGE            100
#define M_FORGET_DIST       SQUARE(WALL_L * 7) // = 51380224
#define M_AWARE_DIST        SQUARE(WALL_L * 3) // = 9437184
#define M_STAND_ATTACK_DIST SQUARE(WALL_L / 2) // = 262144
#define M_WALK_ATTACK_DIST  SQUARE((WALL_L / 2) + M_RADIUS) // = 465124
#define M_WALK_DIST         SQUARE(WALL_L) // = 1048576
#define M_WALK_TURN         (DEG_1 * 7) // = 1274
#define M_GET_UP_CHANCE     0x7F
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_PUSHED_BACK         = 3,
    M_ANIM_COLLAPSE            = 10,
    M_ANIM_LYING_DOWN          = 12,
    M_ANIM_HIT_LEFT            = 15,
    M_ANIM_ARMS_CROSSED        = 19,
    M_ANIM_ARMS_UP_PUSHED_BACK = 20,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_ARMS_CROSSED,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_WALK_ARMS_UP,
    M_STATE_WALK_HIT,
    M_STATE_PUSHED_BACK,
    M_STATE_ARMS_UP_PUSHED_BACK,
    M_STATE_COLLAPSE,
    M_STATE_LYING_DOWN,
    M_STATE_GET_UP,
    M_STATE_HIT,
} M_STATE;

typedef struct {
    int32_t damage;
    int32_t delay;
    int32_t timer;
    bool start_lying_down;
    bool walk_straight;
} M_PRIV;

static const BITE m_LeftHand = {
    .pos = {},
    .mesh_num = 11,
};

static const BITE m_RightHand = {
    .pos = {},
    .mesh_num = 14,
};

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "timer", &p->timer));
    MUST(JSON_READ_OPT(io, "walk_straight", &p->walk_straight));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "timer", p->timer);
    JSONW_WRITE(io, "walk_straight", p->walk_straight);
}

static const char *M_CheckWhole(const TRX_VALUE *const in)
{
    return in->as_int < 0 ? "value is below zero" : nullptr;
}

static void M_SetInitialAnim(ITEM *const item, const M_ANIM anim_idx)
{
    Item_SwitchToAnim(item, anim_idx, 0);
    Item_SetVisible(item, anim_idx == M_ANIM_LYING_DOWN);

    const ANIM *const anim = Item_GetAnim(item);
    item->current_anim_state = anim->current_anim_state;
    item->goal_anim_state = anim->current_anim_state;
}

static void M_SetStartLyingDown(ITEM *const item, const TRX_VALUE *const in)
{
    if (item->hit_points <= 0 || item->is_simulated
        || !Item_TestFrameEqual(item, 0)) {
        return;
    }

    if (in->as_bool && !Item_TestAnimEqual(item, M_ANIM_ARMS_CROSSED)) {
        return;
    }

    if (!in->as_bool && !Item_TestAnimEqual(item, M_ANIM_LYING_DOWN)) {
        return;
    }

    M_SetInitialAnim(
        item, in->as_bool ? M_ANIM_LYING_DOWN : M_ANIM_ARMS_CROSSED);
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    M_SetInitialAnim(Item_Get(item_num), M_ANIM_ARMS_CROSSED);
}

static bool M_IsTargetable(const ITEM *const item)
{
    return Item_IsAlive(item) && item->current_anim_state != M_STATE_LYING_DOWN;
}

static ITEM_HIT_EFFECT M_GetHitEffect(const ITEM *const item)
{
    return ITEM_HIT_SMOKE;
}

static bool M_PushBack(
    ITEM *const item, CREATURE *const creature, const AI_INFO *const info)
{
    if (!item->hit_status || info->distance >= M_AWARE_DIST) {
        return false;
    }

    switch (item->current_anim_state) {
    case M_STATE_PUSHED_BACK:
    case M_STATE_COLLAPSE:
    case M_STATE_LYING_DOWN:
        return false;
    default:
        break;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const bool has_relevant_gun = lara->gun_type == LGT_SHOTGUN
        || lara->gun_type == LGT_GRENADE || lara->gun_type == LGT_REVOLVER;

    if ((Random_GetControl() & 3) == 0 && has_relevant_gun) {
        Item_SwitchToAnim(item, M_ANIM_COLLAPSE, 0);
        item->current_anim_state = M_STATE_COLLAPSE;
        creature->maximum_turn = 0;
        item->rot.y += info->angle;
    } else if ((Random_GetControl() & 7) == 0 || has_relevant_gun) {
        if (item->current_anim_state == M_STATE_WALK_ARMS_UP
            || item->current_anim_state == M_STATE_WALK_HIT) {
            Item_SwitchToAnim(item, M_ANIM_ARMS_UP_PUSHED_BACK, 0);
            item->current_anim_state = M_STATE_ARMS_UP_PUSHED_BACK;
        } else {
            Item_SwitchToAnim(item, M_ANIM_PUSHED_BACK, 0);
            item->current_anim_state = M_STATE_PUSHED_BACK;
        }
        item->rot.y += info->angle;
    }

    return true;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;

    item->hit_points = item->max_hit_points;

    ITEM *const lara_item = Lara_GetItem();
    if (item->ai_bits != 0) {
        Creature_GetAITarget(creature);
    } else if (creature->hurt_by_lara) {
        creature->enemy = lara_item;
    }

    AI_INFO info = {};
    Creature_AIInfo(item, &info);

    if (M_PushBack(item, creature, &info)) {
        goto finish;
    }

    Creature_Mood(item, &info, true);
    angle = Creature_Turn(item, creature->maximum_turn);

    if (info.ahead) {
        head = info.angle >> 1;
        torso_y = info.angle >> 1;
        torso_x = info.x_angle;
    }

    M_PRIV *const p = item->priv;
    switch (item->current_anim_state) {
    case M_STATE_ARMS_CROSSED:
        creature->maximum_turn = 0;
        if (info.distance < M_WALK_DIST || p->timer >= p->delay) {
            item->goal_anim_state = M_STATE_WALK;
        }
        break;

    case M_STATE_STOP:
        creature->flags = 0;
        creature->maximum_turn = 0;

        if (info.distance > M_STAND_ATTACK_DIST
            && info.distance < M_FORGET_DIST) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (info.distance < M_STAND_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_HIT;
        } else {
            head = 0;
            torso_x = 0;
            torso_y = 0;
            item->goal_anim_state = M_STATE_STOP;

            if (p->timer < p->delay) {
                p->timer++;
            }
        }
        break;

    case M_STATE_WALK:
        if (p->walk_straight) {
            creature->maximum_turn = 0;
            p->walk_straight = !Item_TestFrameEqual(item, -1);
        } else {
            creature->maximum_turn = M_WALK_TURN;
            if (info.distance < M_AWARE_DIST) {
                item->goal_anim_state = M_STATE_WALK_ARMS_UP;
            } else if (info.distance > M_FORGET_DIST) {
                item->goal_anim_state = M_STATE_STOP;
            }
        }
        break;

    case M_STATE_WALK_ARMS_UP:
        creature->flags = 0;
        creature->maximum_turn = M_WALK_TURN;

        if (info.distance < M_STAND_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.distance > M_AWARE_DIST) {
            if (info.distance < M_FORGET_DIST) {
                item->goal_anim_state = M_STATE_WALK;
                break;
            }
        } else if (info.distance > M_FORGET_DIST) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.distance < M_WALK_ATTACK_DIST) {
            item->goal_anim_state = M_STATE_WALK_HIT;
        }
        break;

    case M_STATE_WALK_HIT:
    case M_STATE_HIT:
        creature->maximum_turn = 0;
        if (ABS(info.angle) < M_WALK_TURN) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= M_WALK_TURN;
        } else {
            item->rot.y += M_WALK_TURN;
        }

        if (creature->flags == 0 && (item->touch_bits & M_TOUCH_BITS) != 0
            && Item_TestFrameRange(item, 14, 21)) {
            Lara_TakeDamage(p->damage, true);
            const BITE *const hand = Item_TestAnimEqual(item, M_ANIM_HIT_LEFT)
                ? &m_LeftHand
                : &m_RightHand;
            Creature_EffectEx(item, hand, 5, -1, Spawn_Blood);
            creature->flags = 1;
        }
        break;

    case M_STATE_LYING_DOWN:
        head = 0;
        torso_x = 0;
        torso_y = 0;
        creature->maximum_turn = 0;

        if (info.distance < M_WALK_DIST
            || (Random_GetControl() & M_GET_UP_CHANCE) == 0) {
            item->goal_anim_state = M_STATE_GET_UP;
        }
        break;
    }

finish:
    Creature_Tilt(item, 0);
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
    obj->is_targetable_func = M_IsTargetable;
    obj->get_hit_effect_func = M_GetHitEffect;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = M_PIVOT;
    obj->radius = M_RADIUS;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->intelligent = true;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_hitpoints = true;
    obj->save_position = true;

    Object_GetBone(obj, 7)->rot.x = true;
    Object_GetBone(obj, 7)->rot.y = true;
    Object_GetBone(obj, 18)->rot.y = true;

    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the mummy's attack."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, delay, 0, M_CheckWhole,
            "Delay before the mummy becomes active."),
        OBJECT_PROPERTY_SETTER(
            M_PRIV, start_lying_down, false, nullptr, M_SetStartLyingDown,
            "Whether the mummy starts lying down."),
        OBJECT_PROPERTY(
            M_PRIV, walk_straight, false,
            "Whether the mummy initially walks straight ahead."));
}

REGISTER_OBJECT(O_UNDEAD_MUMMY, M_Setup)
