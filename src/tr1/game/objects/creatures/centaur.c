#include "game/creature.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define CENTAUR_PART_DAMAGE 100
#define CENTAUR_REAR_DAMAGE 200
#define CENTAUR_TOUCH 0x30199
#define CENTAUR_DIE_ANIM 8
#define CENTAUR_TURN (DEG_1 * 4) // = 728
#define CENTAUR_REAR_CHANCE 96
#define CENTAUR_REAR_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define CENTAUR_HITPOINTS 120
#define CENTAUR_RADIUS (WALL_L / 3) // = 341
#define CENTAUR_SMARTNESS 0x7FFF

typedef enum {
    CENTAUR_STATE_EMPTY = 0,
    CENTAUR_STATE_STOP = 1,
    CENTAUR_STATE_SHOOT = 2,
    CENTAUR_STATE_RUN = 3,
    CENTAUR_STATE_AIM = 4,
    CENTAUR_STATE_DEATH = 5,
    CENTAUR_STATE_WARNING = 6,
} CENTAUR_STATE;

static BITE m_CentaurRocket = { 11, 415, 41, 13 };
static BITE m_CentaurRear = { 50, 30, 0, 5 };

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = CENTAUR_HITPOINTS;
    obj->pivot_length = 400;
    obj->radius = CENTAUR_RADIUS;
    obj->smartness = CENTAUR_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 10)->rot_x = true;
    Object_GetBone(obj, 10)->rot_y = true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *centaur = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CENTAUR_STATE_DEATH) {
            item->current_anim_state = CENTAUR_STATE_DEATH;
            Item_SwitchToAnim(item, CENTAUR_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, CENTAUR_TURN);

        switch (item->current_anim_state) {
        case CENTAUR_STATE_STOP:
            centaur->neck_rotation = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->goal_anim_state = CENTAUR_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = CENTAUR_STATE_AIM;
            } else {
                item->goal_anim_state = CENTAUR_STATE_RUN;
            }
            break;

        case CENTAUR_STATE_RUN:
            if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->required_anim_state = CENTAUR_STATE_WARNING;
                item->goal_anim_state = CENTAUR_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = CENTAUR_STATE_AIM;
                item->goal_anim_state = CENTAUR_STATE_STOP;
            } else if (Random_GetControl() < CENTAUR_REAR_CHANCE) {
                item->required_anim_state = CENTAUR_STATE_WARNING;
                item->goal_anim_state = CENTAUR_STATE_STOP;
            }
            break;

        case CENTAUR_STATE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = CENTAUR_STATE_SHOOT;
            } else {
                item->goal_anim_state = CENTAUR_STATE_STOP;
            }
            break;

        case CENTAUR_STATE_SHOOT:
            if (item->required_anim_state == CENTAUR_STATE_EMPTY) {
                item->required_anim_state = CENTAUR_STATE_AIM;
                int16_t effect_num =
                    Creature_Effect(item, &m_CentaurRocket, Spawn_RocketGun);
                if (effect_num != NO_EFFECT) {
                    centaur->neck_rotation = Effect_Get(effect_num)->rot.x;
                }
            }
            break;

        case CENTAUR_STATE_WARNING:
            if (item->required_anim_state == CENTAUR_STATE_EMPTY
                && (item->touch_bits & CENTAUR_TOUCH)) {
                Creature_Effect(item, &m_CentaurRear, Spawn_Blood);
                Lara_TakeDamage(CENTAUR_REAR_DAMAGE, true);
                item->required_anim_state = CENTAUR_STATE_STOP;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);

    if (item->status == IS_DEACTIVATED) {
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        Item_Explode(item_num, -1, CENTAUR_PART_DAMAGE);
        Item_Kill(item_num);
        item->status = IS_DEACTIVATED;
    }
}

REGISTER_OBJECT(O_CENTAUR, M_Setup)
