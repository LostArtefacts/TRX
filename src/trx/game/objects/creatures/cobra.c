#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS      8
#define M_DAMAGE          80
#define M_ALERT_RANGE     1.5
#define M_ATTACK_RANGE    1.0
#define M_FORGET_RANGE    3.0
#define M_RADIUS          (WALL_L / 10) // = 102
#define M_SQ_SECTORS(r)   (SQUARE(WALL_L * (r)))
// clang-format on

typedef enum {
    M_STATE_WAKING_UP,
    M_STATE_ALERT,
    M_STATE_BITE,
    M_STATE_SLEEP,
    M_STATE_DEATH,
} M_STATE;

typedef enum {
    M_ANIM_SLEEP = 2,
    M_ANIM_DEATH = 4,
} M_ANIM;

typedef struct {
    int32_t damage;
    float alert_radius;
    float attack_radius;
    float forget_radius;
} M_PRIV;

static BITE m_CobraBite = {
    .pos = { 0, 0, 0 },
    .mesh_num = 13,
};

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Creature_Initialise(item_num);
    Item_SwitchToAnim(item, M_ANIM_SLEEP, 45);
    item->current_anim_state = M_STATE_SLEEP;
    item->goal_anim_state = M_STATE_SLEEP;
}

static bool M_IsTargetable(const ITEM *const item)
{
    return item->hit_points > 0 && Item_IsInPlay(item)
        && item->current_anim_state != M_STATE_SLEEP;
}

static bool M_CanTakeDamage(const ITEM *const item)
{
    return item->hit_points > 0;
}

static bool M_CanBeProjectileTarget(const ITEM *const item)
{
    return item->hit_points > 0 && item->is_collidable;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;

    if (creature == nullptr) {
        return;
    }

    int16_t angle = 0;
    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
            item->current_anim_state = M_ANIM_DEATH;
        }

        goto finish;
    }

    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    AI_INFO info;
    Creature_AIInfo(item, &info);
    info.angle += 3072;
    creature->target.x = lara_item->pos.x;
    creature->target.z = lara_item->pos.z;
    angle = Creature_Turn(item, creature->maximum_turn);

    if (ABS(info.angle) < DEG_1 * 10) {
        item->rot.y += info.angle;
    } else if (info.angle < 0) {
        item->rot.y -= DEG_1 * 10;
    } else {
        item->rot.y += DEG_1 * 10;
    }

    switch (item->current_anim_state) {
    case M_STATE_WAKING_UP:
        break;

    case M_STATE_ALERT:
        creature->flags = 0;
        if (info.distance > M_SQ_SECTORS(p->forget_radius)) {
            item->goal_anim_state = M_STATE_SLEEP;
        } else if (
            lara_item->hit_points > 0
            && ((info.ahead && info.distance < M_SQ_SECTORS(p->attack_radius))
                || item->hit_status || lara_item->speed > 15)) {
            item->goal_anim_state = M_STATE_BITE;
        }
        break;

    case M_STATE_BITE:
        if (creature->flags != 1 && (item->touch_bits & 0x2000) != 0) {
            creature->flags = 1;
            Lara_TakeDamage(p->damage, true);
            lara->poison.value = 256;
            Creature_Effect(item, &m_CobraBite, Spawn_Blood);
        }
        break;

    case M_STATE_SLEEP:
        creature->flags = 0;
        if (info.distance < M_SQ_SECTORS(p->alert_radius)
            && lara_item->hit_points > 0) {
            item->goal_anim_state = M_STATE_WAKING_UP;
        }
        break;
    }

finish:
    Creature_Animate(item_num, angle, 0);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->is_targetable_func = M_IsTargetable;
    obj->can_take_damage_func = M_CanTakeDamage;
    obj->can_be_projectile_target_func = M_CanBeProjectileTarget;

    obj->priv_size = sizeof(M_PRIV);
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->radius = M_RADIUS;

    // obj->non_lot = true; // TODO(TR3)
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;

    Object_GetBone(obj, 0)->rot.y = true;
    Object_GetBone(obj, 6)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DAMAGE, "Damage dealt by the cobra bite."),
        OBJECT_PROPERTY(
            M_PRIV, alert_radius, M_ALERT_RANGE, "Alert radius, in sectors."),
        OBJECT_PROPERTY(
            M_PRIV, attack_radius, M_ATTACK_RANGE,
            "Attack radius, in sectors."),
        OBJECT_PROPERTY(
            M_PRIV, forget_radius, M_FORGET_RANGE,
            "Forget radius, in sectors."));
}

REGISTER_OBJECT(O_COBRA, M_Setup)
