#include "game/objects/creatures/cultist_3.h"

#include "game/creature.h"
#include "game/objects/creatures/cultist_common.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/utils.h>

// clang-format off
#define CULTIST_3_HITPOINTS   150
#define CULTIST_3_SHOT_DAMAGE 50
#define CULTIST_3_WALK_TURN   (DEG_1 * 3) // = 546
#define CULTIST_3_RUN_TURN    (DEG_1 * 3) // = 546
#define CULTIST_3_STOP_RANGE  SQUARE(WALL_L * 3) // = 9437184
#define CULTIST_3_RUN_RANGE   SQUARE(WALL_L * 5) // = 26214400
// clang-format on

typedef enum {
    // clang-format off
    CULTIST_3_STATE_EMPTY   = 0,
    CULTIST_3_STATE_STOP    = 1,
    CULTIST_3_STATE_WAIT    = 2,
    CULTIST_3_STATE_WALK    = 3,
    CULTIST_3_STATE_RUN     = 4,
    CULTIST_3_STATE_AIM_L   = 5,
    CULTIST_3_STATE_AIM_R   = 6,
    CULTIST_3_STATE_SHOOT_L = 7,
    CULTIST_3_STATE_SHOOT_R = 8,
    CULTIST_3_STATE_AIM_2   = 9,
    CULTIST_3_STATE_SHOOT_2 = 10,
    CULTIST_3_STATE_DEATH   = 11,
    // clang-format on
} CULTIST_3_STATE;

typedef enum {
    // clang-format off
    CULTIST_3_ANIM_WAIT  = 3,
    CULTIST_3_ANIM_DEATH = 32,
    // clang-format on
} CULTIST_3_ANIM;

static const BITE m_Cultist3LeftGun = {
    .pos = { .x = -2, .y = 275, .z = 23 },
    .mesh_num = 6,
};

static const BITE m_Cultist3RightGun = {
    .pos = { .x = 2, .y = 275, .z = 23 },
    .mesh_num = 10,
};

void Cultist3_Setup(void)
{
    OBJECT *const obj = Object_Get(O_CULT_3);
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = Cultist3_Initialise;
    obj->control_func = Cultist3_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = CULTIST_3_HITPOINTS;
    obj->radius = CULTIST_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Cultist3_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, CULTIST_3_ANIM_WAIT, 0);
    item->goal_anim_state = CULTIST_3_STATE_WAIT;
    item->current_anim_state = CULTIST_3_STATE_WAIT;
}

void Cultist3_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t head = 0;
    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t body = 0;
    int16_t left = 0;
    int16_t right = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CULTIST_3_STATE_DEATH) {
            Item_SwitchToAnim(item, CULTIST_3_ANIM_DEATH, 0);
            item->current_anim_state = CULTIST_3_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_ATTACK);

        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case CULTIST_3_STATE_STOP:
        case CULTIST_3_STATE_WAIT:
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (creature->mood == MOOD_BORED && g_LaraItem->hit_points <= 0) {
                item->goal_anim_state = CULTIST_3_STATE_WAIT;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance > CULTIST_3_STOP_RANGE) {
                    item->goal_anim_state = CULTIST_3_STATE_WALK;
                } else {
                    item->goal_anim_state = CULTIST_3_STATE_AIM_2;
                }
            } else if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CULTIST_3_STATE_RUN;
            } else if (creature->mood == MOOD_ATTACK) {
                if (info.distance > CULTIST_3_RUN_RANGE || info.ahead == 0) {
                    item->goal_anim_state = CULTIST_3_STATE_RUN;
                } else {
                    item->goal_anim_state = CULTIST_3_STATE_WALK;
                }
            } else if (creature->mood == MOOD_STALK || info.ahead == 0) {
                item->goal_anim_state = CULTIST_3_STATE_WALK;
            }
            break;

        case CULTIST_3_STATE_WALK:
            creature->maximum_turn = CULTIST_3_WALK_TURN;
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance < CULTIST_3_STOP_RANGE
                    || info.zone_num != info.enemy_zone_num) {
                    item->goal_anim_state = CULTIST_3_STATE_STOP;
                } else if (info.angle < 0) {
                    item->goal_anim_state = CULTIST_3_STATE_AIM_L;
                } else {
                    item->goal_anim_state = CULTIST_3_STATE_AIM_R;
                }
            } else if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = CULTIST_3_STATE_RUN;
            } else if (
                creature->mood == MOOD_STALK || creature->mood == MOOD_ATTACK) {
                if (info.distance > CULTIST_3_RUN_RANGE || info.ahead == 0) {
                    item->goal_anim_state = CULTIST_3_STATE_RUN;
                }
            } else if (g_LaraItem->hit_points <= 0) {
                item->goal_anim_state = CULTIST_3_STATE_WAIT;
            } else if (info.ahead != 0) {
                item->goal_anim_state = CULTIST_3_STATE_STOP;
            }
            break;

        case CULTIST_3_STATE_RUN:
            creature->maximum_turn = CULTIST_3_RUN_TURN;
            tilt = angle / 4;
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (Creature_CanTargetEnemy(item, &info)) {
                if (info.zone_num != info.enemy_zone_num) {
                    item->goal_anim_state = CULTIST_3_STATE_STOP;
                } else if (info.angle < 0) {
                    item->goal_anim_state = CULTIST_3_STATE_AIM_L;
                } else {
                    item->goal_anim_state = CULTIST_3_STATE_AIM_R;
                }
            } else if (creature->mood == MOOD_BORED) {
                if (g_LaraItem->hit_points <= 0) {
                    item->goal_anim_state = CULTIST_3_STATE_WAIT;
                } else {
                    item->goal_anim_state = CULTIST_3_STATE_STOP;
                }
            } else if (info.ahead != 0 && info.distance < CULTIST_3_RUN_RANGE) {
                item->goal_anim_state = CULTIST_3_STATE_WALK;
            }
            break;

        case CULTIST_3_STATE_AIM_L:
            creature->flags = 0;
            if (info.ahead != 0) {
                head = info.angle;
                left = info.angle;
            }
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = CULTIST_3_STATE_SHOOT_L;
            } else {
                item->goal_anim_state = CULTIST_3_STATE_WALK;
            }
            break;

        case CULTIST_3_STATE_AIM_R:
            creature->flags = 0;
            if (info.ahead != 0) {
                head = info.angle;
                right = info.angle;
            }
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = CULTIST_3_STATE_SHOOT_R;
            } else {
                item->goal_anim_state = CULTIST_3_STATE_WALK;
            }
            break;

        case CULTIST_3_STATE_AIM_2:
            creature->flags = 0;
            if (info.ahead != 0) {
                body = info.angle;
            }
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = CULTIST_3_STATE_SHOOT_2;
            } else {
                item->goal_anim_state = CULTIST_3_STATE_STOP;
            }
            break;

        case CULTIST_3_STATE_SHOOT_L:
            if (info.ahead != 0) {
                head = info.angle;
                left = info.angle;
            }
            if (creature->flags == 0) {
                Creature_ShootAtLara(
                    item, &info, &m_Cultist3LeftGun, head,
                    CULTIST_3_SHOT_DAMAGE);
                creature->flags = 1;
            }
            break;

        case CULTIST_3_STATE_SHOOT_R:
            if (info.ahead != 0) {
                head = info.angle;
                right = info.angle;
            }
            if (creature->flags == 0) {
                Creature_ShootAtLara(
                    item, &info, &m_Cultist3RightGun, head,
                    CULTIST_3_SHOT_DAMAGE);
                creature->flags = 1;
            }
            break;

        case CULTIST_3_STATE_SHOOT_2:
            if (info.ahead != 0) {
                body = info.angle;
            }
            if (creature->flags == 0) {
                Creature_ShootAtLara(
                    item, &info, &m_Cultist3LeftGun, 0, CULTIST_3_SHOT_DAMAGE);
                Creature_ShootAtLara(
                    item, &info, &m_Cultist3RightGun, 0, CULTIST_3_SHOT_DAMAGE);
                creature->flags = 1;
            }
            break;

        default:
            break;
        }
    }

    Creature_Tilt(item, tilt);

    const OBJECT *const obj = Object_Get(item->object_id);
    Object_GetBone(obj, 0)->rot_y = body != 0;
    Object_GetBone(obj, 2)->rot_y = left != 0;
    Object_GetBone(obj, 6)->rot_y = right != 0;
    Object_GetBone(obj, 10)->rot_y = head != 0;

    if (body != 0) {
        Creature_Head(item, body);
    } else if (left != 0) {
        Creature_Head(item, left);
        Creature_Neck(item, head);
    } else if (right != 0) {
        Creature_Head(item, right);
        Creature_Neck(item, head);
    } else if (head != 0) {
        Creature_Head(item, head);
    }

    Creature_Animate(item_num, angle, 0);
}
