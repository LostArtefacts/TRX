#include "game/carrier.h"
#include "game/creature.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/room.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define RAT_BITE_DAMAGE 20
#define RAT_CHARGE_DAMAGE 20
#define RAT_TOUCH 0x300018F
#define RAT_DIE_ANIM 8
#define RAT_RUN_TURN (DEG_1 * 6) // = 1092
#define RAT_BITE_RANGE SQUARE(341) // = 116281
#define RAT_CHARGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAT_POSE_CHANCE 256
#define RAT_HITPOINTS 5
#define RAT_RADIUS (WALL_L / 5) // = 204
#define RAT_SMARTNESS 0x2000

#define VOLE_DIE_ANIM 2
#define VOLE_SWIM_TURN (DEG_1 * 3) // = 546
#define VOLE_ATTACK_RANGE SQUARE(300) // = 90000

typedef enum {
    RAT_STATE_EMPTY = 0,
    RAT_STATE_STOP = 1,
    RAT_STATE_ATTACK_2 = 2,
    RAT_STATE_RUN = 3,
    RAT_STATE_ATTACK_1 = 4,
    RAT_STATE_DEATH = 5,
    RAT_STATE_POSE = 6,
} RAT_STATE;

typedef enum {
    VOLE_STATE_EMPTY = 0,
    VOLE_STATE_SWIM = 1,
    VOLE_STATE_ATTACK = 2,
    VOLE_STATE_DEATH = 3,
} VOLE_STATE;

static BITE m_RatBite = { 0, -11, 108, 3 };

static void M_SetupBase(OBJECT *obj);
static void M_SetupRat(OBJECT *obj);
static void M_SetupVole(OBJECT *obj);
static void M_ControlRat(int16_t item_num);
static void M_ControlVole(int16_t item_num);

static const HYBRID_INFO m_RatInfo = {
    .land.id = O_RAT,
    .land.active_anim = RAT_STATE_EMPTY,
    .land.death_anim = RAT_DIE_ANIM,
    .land.death_state = RAT_STATE_DEATH,
    .water.id = O_VOLE,
    .water.active_anim = VOLE_STATE_EMPTY,
};

static void M_SetupBase(OBJECT *const obj)
{
    obj->initialise_func = Creature_Initialise;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = RAT_HITPOINTS;
    obj->pivot_length = 200;
    obj->radius = RAT_RADIUS;
    obj->smartness = RAT_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    Object_GetBone(obj, 1)->rot_y = true;
}

static void M_SetupRat(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);
    obj->control_func = M_ControlRat;
}

static void M_SetupVole(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    M_SetupBase(obj);
    obj->control_func = M_ControlVole;
}

static void M_ControlRat(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *rat = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != RAT_STATE_DEATH) {
            item->current_anim_state = RAT_STATE_DEATH;
            Item_SwitchToAnim(item, RAT_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, RAT_RUN_TURN);

        switch (item->current_anim_state) {
        case RAT_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < RAT_BITE_RANGE) {
                item->goal_anim_state = RAT_STATE_ATTACK_1;
            } else {
                item->goal_anim_state = RAT_STATE_RUN;
            }
            break;

        case RAT_STATE_RUN:
            if (info.ahead && (item->touch_bits & RAT_TOUCH)) {
                item->goal_anim_state = RAT_STATE_STOP;
            } else if (info.bite && info.distance < RAT_CHARGE_RANGE) {
                item->goal_anim_state = RAT_STATE_ATTACK_2;
            } else if (info.ahead && Random_GetControl() < RAT_POSE_CHANCE) {
                item->required_anim_state = RAT_STATE_POSE;
                item->goal_anim_state = RAT_STATE_STOP;
            }
            break;

        case RAT_STATE_ATTACK_1:
            if (item->required_anim_state == RAT_STATE_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                Creature_Effect(item, &m_RatBite, Spawn_Blood);
                Lara_TakeDamage(RAT_BITE_DAMAGE, true);
                item->required_anim_state = RAT_STATE_STOP;
            }
            break;

        case RAT_STATE_ATTACK_2:
            if (item->required_anim_state == RAT_STATE_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                Creature_Effect(item, &m_RatBite, Spawn_Blood);
                Lara_TakeDamage(RAT_CHARGE_DAMAGE, true);
                item->required_anim_state = RAT_STATE_RUN;
            }
            break;

        case RAT_STATE_POSE:
            if (rat->mood != MOOD_BORED
                || Random_GetControl() < RAT_POSE_CHANCE) {
                item->goal_anim_state = RAT_STATE_STOP;
            }
            break;
        }
    }

    Creature_Head(item, head);

    int32_t wh;
    Creature_EnsureHabitat(item_num, &wh, &m_RatInfo);

    Creature_Animate(item_num, angle, 0);
}

static void M_ControlVole(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != VOLE_STATE_DEATH) {
            item->current_anim_state = VOLE_STATE_DEATH;
            Item_SwitchToAnim(item, VOLE_DIE_ANIM, 0);
            Carrier_TestItemDrops(item_num);
        }

        Creature_Head(item, head);

        Item_Animate(item);

        int32_t wh = Room_GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_num);
        if (wh == NO_HEIGHT) {
            item->object_id = O_RAT;
            item->current_anim_state = RAT_STATE_DEATH;
            item->goal_anim_state = RAT_STATE_DEATH;
            Item_SwitchToAnim(item, RAT_DIE_ANIM, -1);
            item->pos.y = item->floor;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, VOLE_SWIM_TURN);

        switch (item->current_anim_state) {
        case VOLE_STATE_SWIM:
            if (info.ahead && (item->touch_bits & RAT_TOUCH)) {
                item->goal_anim_state = VOLE_STATE_ATTACK;
            }
            break;

        case VOLE_STATE_ATTACK:
            if (item->required_anim_state == VOLE_STATE_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                Creature_Effect(item, &m_RatBite, Spawn_Blood);
                Lara_TakeDamage(RAT_BITE_DAMAGE, true);
                item->required_anim_state = VOLE_STATE_SWIM;
            }
            item->goal_anim_state = VOLE_STATE_EMPTY;
            break;
        }

        Creature_Head(item, head);

        int32_t wh;
        Creature_EnsureHabitat(item_num, &wh, &m_RatInfo);

        int32_t height = item->pos.y;
        item->pos.y = item->floor;

        Creature_Animate(item_num, angle, 0);

        if (height != NO_HEIGHT) {
            if (wh - height < -STEP_L / 8) {
                item->pos.y = height - STEP_L / 8;
            } else if (wh - height > STEP_L / 8) {
                item->pos.y = height + STEP_L / 8;
            } else {
                item->pos.y = wh;
            }
        }
    }
}

REGISTER_OBJECT(O_RAT, M_SetupRat)
REGISTER_OBJECT(O_VOLE, M_SetupVole)
