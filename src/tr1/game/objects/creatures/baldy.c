#include "game/objects/creatures/baldy.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lot.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define BALDY_SHOT_DAMAGE 150
#define BALDY_WALK_TURN (DEG_1 * 3) // = 546
#define BALDY_RUN_TURN (DEG_1 * 6) // = 1092
#define BALDY_WALK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define BALDY_DIE_ANIM 14
#define BALDY_HITPOINTS 200
#define BALDY_RADIUS (WALL_L / 10) // = 102
#define BALDY_SMARTNESS 0x7FFF

typedef enum {
    BALDY_STATE_EMPTY = 0,
    BALDY_STATE_STOP = 1,
    BALDY_STATE_WALK = 2,
    BALDY_STATE_RUN = 3,
    BALDY_STATE_AIM = 4,
    BALDY_STATE_DEATH = 5,
    BALDY_STATE_SHOOT = 6,
} BALDY_STATE;

static BITE m_BaldyGun = { -20, 440, 20, 9 };

void Baldy_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Baldy_Initialise;
    obj->control_func = Baldy_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = BALDY_HITPOINTS;
    obj->radius = BALDY_RADIUS;
    obj->smartness = BALDY_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 0)->rot_y = true;
}

void Baldy_Initialise(int16_t item_num)
{
    Creature_Initialise(item_num);
    Item_Get(item_num)->current_anim_state = BALDY_STATE_RUN;
}

void Baldy_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *baldy = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != BALDY_STATE_DEATH) {
            item->current_anim_state = BALDY_STATE_DEATH;
            Item_SwitchToAnim(item, BALDY_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, baldy->maximum_turn);

        switch (item->current_anim_state) {
        case BALDY_STATE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = BALDY_STATE_AIM;
            } else if (baldy->mood == MOOD_BORED) {
                item->goal_anim_state = BALDY_STATE_WALK;
            } else {
                item->goal_anim_state = BALDY_STATE_RUN;
            }
            break;

        case BALDY_STATE_WALK:
            baldy->maximum_turn = BALDY_WALK_TURN;
            if (baldy->mood == MOOD_ESCAPE || !info.ahead) {
                item->required_anim_state = BALDY_STATE_RUN;
                item->goal_anim_state = BALDY_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = BALDY_STATE_AIM;
                item->goal_anim_state = BALDY_STATE_STOP;
            } else if (info.distance > BALDY_WALK_RANGE) {
                item->required_anim_state = BALDY_STATE_RUN;
                item->goal_anim_state = BALDY_STATE_STOP;
            }
            break;

        case BALDY_STATE_RUN:
            baldy->maximum_turn = BALDY_RUN_TURN;
            tilt = angle / 2;
            if (baldy->mood != MOOD_ESCAPE || info.ahead) {
                if (Creature_CanTargetEnemy(item, &info)) {
                    item->required_anim_state = BALDY_STATE_AIM;
                    item->goal_anim_state = BALDY_STATE_STOP;
                } else if (info.ahead && info.distance < BALDY_WALK_RANGE) {
                    item->required_anim_state = BALDY_STATE_WALK;
                    item->goal_anim_state = BALDY_STATE_STOP;
                }
            }
            break;

        case BALDY_STATE_AIM:
            baldy->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = BALDY_STATE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = BALDY_STATE_SHOOT;
            } else {
                item->goal_anim_state = BALDY_STATE_STOP;
            }
            break;

        case BALDY_STATE_SHOOT:
            if (!baldy->flags) {
                Creature_ShootAtLara(
                    item, info.distance / 2, &m_BaldyGun, head,
                    BALDY_SHOT_DAMAGE);
                baldy->flags = 1;
            }
            if (baldy->mood == MOOD_ESCAPE) {
                item->required_anim_state = BALDY_STATE_RUN;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}
