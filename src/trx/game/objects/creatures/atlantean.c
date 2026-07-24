#include <trx/game/objects/creatures/atlantean.h>

#include <trx/game/creature.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS        50
#define M_RADIUS            (WALL_L / 3)            // = 341
#define M_CHARGE_DAMAGE     100
#define M_LUNGE_DAMAGE      150
#define M_PUNCH_DAMAGE      200
#define M_PART_DAMAGE       100
#define M_WALK_TURN         (DEG_1 * 2)             // = 364
#define M_RUN_TURN          (DEG_1 * 6)             // = 1092
#define M_POSE_CHANCE       80
#define M_UNPOSE_CHANCE     256
#define M_WALK_RANGE        SQUARE(WALL_L * 9 / 2)  // = 21233664
#define M_ATTACK_1_RANGE    SQUARE(600)             // = 360000
#define M_ATTACK_2_RANGE    SQUARE(WALL_L * 5 / 2)  // = 6553600
#define M_ATTACK_3_RANGE    SQUARE(300)             // = 90000
#define M_ATTACK_RANGE      SQUARE(WALL_L * 15 / 4) // = 14745600
#define M_TOUCH_BITS        0b00000110'01111000     // = 0x678
#define M_DEFAULT_SMARTNESS 0x7FFF
#define M_SHOOTER_SMARTNESS 0x2000
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_ATTACK_1,
    M_STATE_DEATH,
    M_STATE_POSE,
    M_STATE_ATTACK_2,
    M_STATE_ATTACK_3,
    M_STATE_AIM_1,
    M_STATE_AIM_2,
    M_STATE_SHOOT,
    M_STATE_MUMMY,
    M_STATE_FLY,
} M_STATE;

typedef enum {
    // clang-format off
    M_FLAG_BULLET_1 = 1 << 0,
    M_FLAG_BULLET_2 = 1 << 1,
    M_FLAG_FLY      = 1 << 2,
    M_FLAG_TWIST    = 1 << 3,
    // clang-format on
} M_FLAG;

static bool m_EnableExplosions = true;
static const BITE m_Bite = { .pos = { -27, 98, 0 }, .mesh_num = 10 };
static const BITE m_Rocket = { .pos = { 51, 213, 0 }, .mesh_num = 14 };
static const BITE m_Shard = { .pos = { -35, 269, 0 }, .mesh_num = 9 };

static int32_t M_GetDamage(
    const ITEM *const item, const char *const key, const int32_t default_value)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, key, &damage)) {
        return damage.as_int;
    }

    return default_value;
}

static void M_InitialiseGround(const int16_t item_num)
{
    Creature_Initialise(item_num);
    Item_Get(item_num)->mesh_bits = 0xFFE07FFF;
}

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        Item_Shatter(
            item_num, -1,
            m_EnableExplosions
                ? M_GetDamage(item, "part_damage", M_PART_DAMAGE)
                : -M_GetDamage(item, "part_damage", M_PART_DAMAGE));
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        LOT_DisableBaddieAI(item_num);
        Item_Destroy(item_num);
        item->is_finished = true;
        Carrier_TestItemDrops(item_num);
        return;
    }
    creature->lot.setup.step = STEP_L;
    creature->lot.setup.drop = -STEP_L;
    creature->lot.setup.fly = 0;

    AI_INFO info;
    Creature_AIInfo(item, &info);

    bool shoot_1 = false;
    bool shoot_2 = false;
    if (item->object_id != O_ATLANTEAN_GROUND
        && Creature_CanTargetEnemy(item, &info)
        && (info.zone_num != info.enemy_zone_num
            || info.distance > M_ATTACK_RANGE)) {
        if (info.angle > 0 && info.angle < DEG_45) {
            shoot_1 = true;
        } else if (info.angle < 0 && info.angle > -DEG_45) {
            shoot_2 = true;
        }
    }

    if (item->object_id == O_ATLANTEAN_WINGED) {
        if (item->current_anim_state == M_STATE_FLY) {
            if ((creature->flags & M_FLAG_FLY) != 0
                && creature->mood != MOOD_ESCAPE
                && info.zone_num == info.enemy_zone_num) {
                creature->flags &= ~M_FLAG_FLY;
            }

            if ((creature->flags & M_FLAG_FLY) == 0) {
                Creature_Mood(item, &info, true);
            }

            creature->lot.setup.step = WALL_L * 30;
            creature->lot.setup.drop = -WALL_L * 30;
            creature->lot.setup.fly = STEP_L / 8;
            Creature_AIInfo(item, &info);
        } else if (
            (info.zone_num != info.enemy_zone_num && !shoot_1 && !shoot_2
             && (!info.ahead || creature->mood == MOOD_BORED))
            || creature->mood == MOOD_ESCAPE) {
            creature->flags |= M_FLAG_FLY;
        }
    }

    if (info.ahead) {
        head = info.angle;
    }

    if (item->current_anim_state != M_STATE_FLY) {
        Creature_Mood(item, &info, false);
    } else if ((creature->flags & M_FLAG_FLY) != 0) {
        Creature_Mood(item, &info, true);
    }

    angle = Creature_Turn(item, creature->maximum_turn);

    switch (item->current_anim_state) {
    case M_STATE_MUMMY:
        item->goal_anim_state = M_STATE_STOP;
        break;

    case M_STATE_STOP:
        creature->flags &= ~(M_FLAG_BULLET_1 | M_FLAG_BULLET_2 | M_FLAG_TWIST);
        if ((creature->flags & M_FLAG_FLY) != 0) {
            item->goal_anim_state = M_STATE_FLY;
        } else if ((item->touch_bits & M_TOUCH_BITS) != 0) {
            item->goal_anim_state = M_STATE_ATTACK_3;
        } else if (info.bite && info.distance < M_ATTACK_3_RANGE) {
            item->goal_anim_state = M_STATE_ATTACK_3;
        } else if (info.bite && info.distance < M_ATTACK_1_RANGE) {
            item->goal_anim_state = M_STATE_ATTACK_1;
        } else if (shoot_1) {
            item->goal_anim_state = M_STATE_AIM_1;
        } else if (shoot_2) {
            item->goal_anim_state = M_STATE_AIM_2;
        } else if (
            creature->mood == MOOD_BORED
            || (creature->mood == MOOD_STALK && info.distance < M_WALK_RANGE)) {
            item->goal_anim_state = M_STATE_POSE;
        } else {
            item->goal_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_POSE:
        head = 0;
        if (shoot_1 || shoot_2 || (creature->flags & M_FLAG_FLY) != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->mood == MOOD_STALK) {
            if (info.distance < M_WALK_RANGE) {
                if (info.zone_num == info.enemy_zone_num
                    || Random_GetControl() < M_UNPOSE_CHANCE) {
                    item->goal_anim_state = M_STATE_WALK;
                }
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (
            creature->mood == MOOD_BORED
            && Random_GetControl() < M_UNPOSE_CHANCE) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (
            creature->mood == MOOD_ATTACK || creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_WALK:
        creature->maximum_turn = M_WALK_TURN;
        if (shoot_1 || shoot_2 || (creature->flags & M_FLAG_FLY) != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            creature->mood == MOOD_ATTACK || creature->mood == MOOD_ESCAPE) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            creature->mood == MOOD_BORED
            || (creature->mood == MOOD_STALK
                && info.zone_num != info.enemy_zone_num)) {
            if (Random_GetControl() < M_POSE_CHANCE) {
                item->goal_anim_state = M_STATE_POSE;
            }
        } else if (
            creature->mood == MOOD_STALK && info.distance > M_WALK_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_RUN:
        creature->maximum_turn = M_RUN_TURN;
        if ((creature->flags & M_FLAG_FLY) != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if ((item->touch_bits & M_TOUCH_BITS) != 0) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.bite && info.distance < M_ATTACK_1_RANGE) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (info.ahead && info.distance < M_ATTACK_2_RANGE) {
            item->goal_anim_state = M_STATE_ATTACK_2;
        } else if (shoot_1 || shoot_2) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            creature->mood == MOOD_BORED
            || (creature->mood == MOOD_STALK && info.distance < M_WALK_RANGE)) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_ATTACK_1:
        if (item->required_anim_state == M_STATE_EMPTY
            && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Creature_Effect(item, &m_Bite, Spawn_Blood);
            Lara_TakeDamage(
                M_GetDamage(item, "lunge_damage", M_LUNGE_DAMAGE), true);
            item->required_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_ATTACK_2:
        if (item->required_anim_state == M_STATE_EMPTY
            && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Creature_Effect(item, &m_Bite, Spawn_Blood);
            Lara_TakeDamage(
                M_GetDamage(item, "charge_damage", M_CHARGE_DAMAGE), true);
            item->required_anim_state = M_STATE_RUN;
        }
        break;

    case M_STATE_ATTACK_3:
        if (item->required_anim_state == M_STATE_EMPTY
            && (item->touch_bits & M_TOUCH_BITS) != 0) {
            Creature_Effect(item, &m_Bite, Spawn_Blood);
            Lara_TakeDamage(
                M_GetDamage(item, "punch_damage", M_PUNCH_DAMAGE), true);
            item->required_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_AIM_1:
        creature->flags |= M_FLAG_TWIST;
        creature->flags |= M_FLAG_BULLET_1;
        if (shoot_1) {
            item->goal_anim_state = M_STATE_SHOOT;
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_AIM_2:
        creature->flags |= M_FLAG_BULLET_2;
        if (shoot_2) {
            item->goal_anim_state = M_STATE_SHOOT;
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;

    case M_STATE_SHOOT:
        if ((creature->flags & M_FLAG_BULLET_1) != 0) {
            creature->flags &= ~M_FLAG_BULLET_1;
            Creature_Effect(item, &m_Shard, Spawn_AtlanteanShard);
        } else if ((creature->flags & M_FLAG_BULLET_2) != 0) {
            creature->flags &= ~M_FLAG_BULLET_2;
            Creature_Effect(item, &m_Rocket, Spawn_AtlanteanBomb);
        }
        break;

    case M_STATE_FLY:
        if ((creature->flags & M_FLAG_FLY) == 0 && item->pos.y == item->floor) {
            item->goal_anim_state = M_STATE_STOP;
        }
        break;
    }

    if ((creature->flags & M_FLAG_TWIST) == 0) {
        creature->head_rotation = creature->neck_rotation;
    }

    Creature_Head(item, head);

    if ((creature->flags & M_FLAG_TWIST) == 0) {
        creature->neck_rotation = creature->head_rotation;
        creature->head_rotation = 0;
    } else {
        creature->neck_rotation = 0;
    }

    Creature_Animate(item_num, angle, 0);
}

static void M_SetupProperties(OBJECT *const obj)
{
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "charge_damage", M_CHARGE_DAMAGE,
            "Damage dealt by the atlantean's charge attack."),
        OBJECT_PROPERTY_INT(
            "lunge_damage", M_LUNGE_DAMAGE,
            "Damage dealt by the atlantean's lunge attack."),
        OBJECT_PROPERTY_INT(
            "punch_damage", M_PUNCH_DAMAGE,
            "Damage dealt by the atlantean's punch attack."),
        OBJECT_PROPERTY_INT(
            "part_damage", M_PART_DAMAGE,
            "Damage dealt by the atlantean's exploding body parts."));
}

static void M_SetupWinged(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;

    obj->pivot_length = 150;
    obj->radius = M_RADIUS;
    obj->smartness = M_DEFAULT_SMARTNESS;
    obj->lot_setup = LOT_Setup(LOT_SETUP_BEAST);
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;
    Object_GetBone(obj, 0)->rot.y = true;
    Object_GetBone(obj, 2)->rot.y = true;
    M_SetupProperties(obj);
}

static void M_SetupShooter(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    *obj = *Object_Get(O_ATLANTEAN_WINGED);
    obj->properties = (OBJECT_PROPERTY_SET) {};
    obj->setup_func = M_SetupShooter;
    obj->initialise_func = M_InitialiseGround;
    obj->smartness = M_SHOOTER_SMARTNESS;
    obj->lot_setup = LOT_Setup(LOT_SETUP_DEFAULT);
    M_SetupProperties(obj);
}

static void M_SetupGround(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    *obj = *Object_Get(O_ATLANTEAN_WINGED);
    obj->properties = (OBJECT_PROPERTY_SET) {};
    obj->setup_func = M_SetupGround;
    obj->initialise_func = M_InitialiseGround;
    obj->lot_setup = LOT_Setup(LOT_SETUP_DEFAULT);
    M_SetupProperties(obj);
}

void Atlantean_ToggleExplosions(bool enable)
{
    m_EnableExplosions = enable;
}

REGISTER_OBJECT(O_ATLANTEAN_WINGED, M_SetupWinged)
REGISTER_OBJECT(O_ATLANTEAN_SHOOTER, M_SetupShooter)
REGISTER_OBJECT(O_ATLANTEAN_GROUND, M_SetupGround)
