#include "game/objects/creatures/bandit_2.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "game/objects/creatures/bandit_common.h"
#include "game/random.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/utils.h>

// clang-format off
#define BANDIT_2_HITPOINTS      50
#define BANDIT_2_SHOOT_DAMAGE   50
#define BANDIT_2_WALK_TURN      (DEG_1 * 4) // = 728
#define BANDIT_2_RUN_TURN       (DEG_1 * 6) // = 1092
#define BANDIT_2_WALK_RANGE     SQUARE(WALL_L * 2) // = 4194304
#define BANDIT_2_WALK_CHANCE    0x4000
#define BANDIT_2_SHOOT_1_CHANCE 0x2000
#define BANDIT_2_SHOOT_2_CHANCE 0x5000
// clang-format on

typedef enum {
    // clang-format off
    BANDIT_2_STATE_EMPTY    = 0,
    BANDIT_2_STATE_AIM_4    = 1,
    BANDIT_2_STATE_WAIT     = 2,
    BANDIT_2_STATE_WALK     = 3,
    BANDIT_2_STATE_RUN      = 4,
    BANDIT_2_STATE_AIM_1    = 5,
    BANDIT_2_STATE_AIM_2    = 6,
    BANDIT_2_STATE_SHOOT_1  = 7,
    BANDIT_2_STATE_SHOOT_2  = 8,
    BANDIT_2_STATE_SHOOT_4A = 9,
    BANDIT_2_STATE_SHOOT_4B = 10,
    BANDIT_2_STATE_DEATH    = 11,
    BANDIT_2_STATE_AIM_5    = 12,
    BANDIT_2_STATE_SHOOT_5  = 13,
    // clang-format on
} BANDIT_2_STATE;

typedef enum {
    BANDIT_2_ANIM_DEATH = 9,
} BANDIT_2_ANIM;

static const BITE m_Bandit2Gun = {
    .pos = { .x = -1, .y = 230, .z = 9 },
    .mesh_num = 17,
};

void Bandit2_Setup(void)
{
    OBJECT *const obj = Object_Get(O_BANDIT_2);
    if (!obj->loaded) {
        return;
    }

    obj->control_func = Bandit2_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = BANDIT_2_HITPOINTS;
    obj->radius = BANDIT_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 6)->rot_y = true;
    Object_GetBone(obj, 8)->rot_y = true;
}

void Bandit2B_Setup(void)
{
    OBJECT *const obj = Object_Get(O_BANDIT_2B);
    if (!obj->loaded) {
        return;
    }

    const OBJECT *const ref_obj = Object_Get(O_BANDIT_2);
    ASSERT(ref_obj->loaded);
    obj->anim_idx = ref_obj->anim_idx;
    obj->frame_base = ref_obj->frame_base;

    obj->control_func = Bandit2_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = BANDIT_2_HITPOINTS;
    obj->radius = BANDIT_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;

    Object_GetBone(obj, 6)->rot_y = true;
    Object_GetBone(obj, 8)->rot_y = true;
}

void Bandit2_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->data;

    int16_t head = 0;
    int16_t tilt = 0;
    int16_t neck = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != BANDIT_2_STATE_DEATH) {
            Item_SwitchToAnim(item, BANDIT_2_ANIM_DEATH, 0);
            item->current_anim_state = BANDIT_2_STATE_DEATH;
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, MOOD_ATTACK);

        angle = Creature_Turn(item, creature->maximum_turn);

        switch (item->current_anim_state) {
        case BANDIT_2_STATE_WAIT:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            creature->maximum_turn = 0;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = BANDIT_2_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance > BANDIT_2_WALK_RANGE
                    && Random_GetControl() < BANDIT_2_WALK_CHANCE) {
                    item->goal_anim_state = BANDIT_2_STATE_WALK;
                } else {
                    const int32_t random = Random_GetControl();
                    if (random < BANDIT_2_SHOOT_1_CHANCE) {
                        item->goal_anim_state = BANDIT_2_STATE_SHOOT_1;
                    } else if (random < BANDIT_2_SHOOT_2_CHANCE) {
                        item->goal_anim_state = BANDIT_2_STATE_SHOOT_2;
                    } else {
                        item->goal_anim_state = BANDIT_2_STATE_AIM_5;
                    }
                }
            } else if (creature->mood == MOOD_BORED) {
                if (info.ahead == 0 || Random_GetControl() < 0x100) {
                    item->goal_anim_state = BANDIT_2_STATE_WALK;
                }
            } else {
                item->goal_anim_state = BANDIT_2_STATE_RUN;
            }
            break;

        case BANDIT_2_STATE_WALK:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            creature->maximum_turn = BANDIT_2_WALK_TURN;
            if (creature->mood == MOOD_ESCAPE) {
                item->goal_anim_state = BANDIT_2_STATE_RUN;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance < BANDIT_2_WALK_RANGE
                    || info.zone_num == info.enemy_zone_num
                    || Random_GetControl() < 0x400) {
                    item->goal_anim_state = BANDIT_2_STATE_WAIT;
                } else {
                    item->goal_anim_state = BANDIT_2_STATE_AIM_4;
                }
            } else if (creature->mood != MOOD_BORED) {
                item->goal_anim_state = BANDIT_2_STATE_RUN;
            } else if (info.ahead && Random_GetControl() < 0x400) {
                item->goal_anim_state = BANDIT_2_STATE_WAIT;
            }
            break;

        case BANDIT_2_STATE_RUN:
            if (info.ahead != 0) {
                neck = info.angle;
            }
            creature->maximum_turn = BANDIT_2_RUN_TURN;
            tilt = angle / 2;
            if (creature->mood == MOOD_ESCAPE) {
            } else if (
                creature->mood == MOOD_BORED
                || Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = BANDIT_2_STATE_WAIT;
            }
            break;

        case BANDIT_2_STATE_AIM_1:
        case BANDIT_2_STATE_AIM_2:
        case BANDIT_2_STATE_AIM_4:
            if (info.ahead != 0) {
                head = info.angle;
            }
            creature->flags = 0;
            break;

        case BANDIT_2_STATE_AIM_5:
            if (info.ahead != 0) {
                head = info.angle;
            }
            creature->flags = 0;
            if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = BANDIT_2_STATE_SHOOT_5;
            } else {
                item->goal_anim_state = BANDIT_2_STATE_WAIT;
            }
            break;

        case BANDIT_2_STATE_SHOOT_1:
        case BANDIT_2_STATE_SHOOT_2:
        case BANDIT_2_STATE_SHOOT_5:
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (creature->flags == 0) {
                if (!Creature_ShootAtLara(
                        item, &info, &m_Bandit2Gun, head, BANDIT_2_SHOOT_DAMAGE)
                    || Random_GetControl() < 0x2000) {
                    item->goal_anim_state = BANDIT_2_STATE_WAIT;
                }
                creature->flags = 1;
            }
            break;

        case BANDIT_2_STATE_SHOOT_4A:
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (creature->flags != 1) {
                if (!Creature_ShootAtLara(
                        item, &info, &m_Bandit2Gun, head,
                        BANDIT_2_SHOOT_DAMAGE)) {
                    item->goal_anim_state = BANDIT_2_STATE_WALK;
                }
                creature->flags = 1;
            }
            if (info.distance < BANDIT_2_WALK_RANGE) {
                item->goal_anim_state = BANDIT_2_STATE_WALK;
            }
            break;

        case BANDIT_2_STATE_SHOOT_4B:
            if (info.ahead != 0) {
                head = info.angle;
            }
            if (creature->flags != 2) {
                if (!Creature_ShootAtLara(
                        item, &info, &m_Bandit2Gun, head,
                        BANDIT_2_SHOOT_DAMAGE)) {
                    item->goal_anim_state = BANDIT_2_STATE_WALK;
                }
                creature->flags = 2;
            }
            if (info.distance < BANDIT_2_WALK_RANGE) {
                item->goal_anim_state = BANDIT_2_STATE_WALK;
            }
            break;

        default:
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Neck(item, neck);
    Creature_Animate(item_num, angle, 0);
}
