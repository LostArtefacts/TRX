#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/lara.h>
#include <trx/game/objects/property.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_HIT_POINTS  120
#define M_PART_DAMAGE 100
#define M_REAR_DAMAGE 200
#define M_TOUCH       0x30199
#define M_RADIUS      (WALL_L / 3) // = 341
#define M_TURN        (DEG_1 * 4) // = 728
#define M_REAR_CHANCE 96
#define M_REAR_RANGE  SQUARE(WALL_L * 3 / 2) // = 2359296
// clang-format on

typedef enum {
    M_STATE_EMPTY,
    M_STATE_STOP,
    M_STATE_SHOOT,
    M_STATE_RUN,
    M_STATE_AIM,
    M_STATE_DEATH,
    M_STATE_WARNING,
} M_STATE;

typedef enum {
    M_ANIM_DEATH = 8,
} M_ANIM;

typedef struct {
    int32_t rear_damage;
    int32_t part_damage;
} M_PRIV;

static BITE m_CentaurRocket = {
    .pos = { 11, 415, 41 },
    .mesh_num = 13,
};
static BITE m_CentaurRear = {
    .pos = { 50, 30, 0 },
    .mesh_num = 5,
};

static void M_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    CREATURE *const centaur = item->creature_data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != M_STATE_DEATH) {
            item->current_anim_state = M_STATE_DEATH;
            Item_SwitchToAnim(item, M_ANIM_DEATH, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, M_TURN);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            centaur->neck_rotation = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < M_REAR_RANGE) {
                item->goal_anim_state = M_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_AIM;
            } else {
                item->goal_anim_state = M_STATE_RUN;
            }
            break;

        case M_STATE_RUN:
            if (info.bite && info.distance < M_REAR_RANGE) {
                item->required_anim_state = M_STATE_WARNING;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = M_STATE_AIM;
                item->goal_anim_state = M_STATE_STOP;
            } else if (Random_GetControl() < M_REAR_CHANCE) {
                item->required_anim_state = M_STATE_WARNING;
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = M_STATE_SHOOT;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
            break;

        case M_STATE_SHOOT:
            if (item->required_anim_state == M_STATE_EMPTY) {
                item->required_anim_state = M_STATE_AIM;
                int16_t effect_num = Creature_Effect(
                    item, &m_CentaurRocket, Spawn_AtlanteanBomb);
                if (effect_num != NO_EFFECT) {
                    centaur->neck_rotation = Effect_Get(effect_num)->rot.x;
                }
            }
            break;

        case M_STATE_WARNING:
            if (item->required_anim_state == M_STATE_EMPTY
                && (item->touch_bits & M_TOUCH)) {
                Creature_Effect(item, &m_CentaurRear, Spawn_Blood);
                Lara_TakeDamage(p->rear_damage, true);
                item->required_anim_state = M_STATE_STOP;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);

    if (item->is_finished) {
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        Item_Shatter(item_num, -1, p->part_damage);
        Item_Destroy(item_num);
        Item_SetFinished(item, true);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->priv_size = sizeof(M_PRIV);
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;

    obj->pivot_length = 400;
    obj->radius = M_RADIUS;
    obj->smartness = 0x7FFF;
    obj->lot_setup = LOT_Setup(LOT_SETUP_BEAST);
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 10)->rot.x = true;
    Object_GetBone(obj, 10)->rot.y = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY(
            M_PRIV, rear_damage, M_REAR_DAMAGE,
            "Damage dealt by the centaur rear attack."),
        OBJECT_PROPERTY(
            M_PRIV, part_damage, M_PART_DAMAGE,
            "Damage dealt by the centaur death explosion."));
}

REGISTER_OBJECT(O_CENTAUR, M_Setup)
