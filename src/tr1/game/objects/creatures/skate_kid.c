#include "game/creature.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/log.h>
#include <libtrx/utils.h>

#define SKATE_KID_STOP_SHOT_DAMAGE 50
#define SKATE_KID_SKATE_SHOT_DAMAGE 40
#define SKATE_KID_STOP_RANGE SQUARE(WALL_L * 4) // = 16777216
#define SKATE_KID_DONT_STOP_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define SKATE_KID_TOO_CLOSE SQUARE(WALL_L) // = 1048576
#define SKATE_KID_SKATE_TURN (DEG_1 * 4) // = 728
#define SKATE_KID_PUSH_CHANCE 0x200
#define SKATE_KID_SKATE_CHANCE 0x400
#define SKATE_KID_DIE_ANIM 13
#define SKATE_KID_HITPOINTS 125
#define SKATE_KID_RADIUS (WALL_L / 5) // = 204
#define SKATE_KID_SMARTNESS 0x7FFF
#define SKATE_KID_SPEECH_HITPOINTS 120
#define SKATE_KID_SPEECH_STARTED 1

typedef enum {
    SKATE_KID_STATE_STOP = 0,
    SKATE_KID_STATE_SHOOT_1 = 1,
    SKATE_KID_STATE_SKATE = 2,
    SKATE_KID_STATE_PUSH = 3,
    SKATE_KID_STATE_SHOOT_2 = 4,
    SKATE_KID_STATE_DEATH = 5,
} SKATE_KID_STATE;

static BITE m_KidGun1 = { 0, 150, 34, 7 };
static BITE m_KidGun2 = { 0, 150, 37, 4 };

static void M_Setup(OBJECT *obj);
static void M_Initialise(int16_t item_num);
static void M_Control(int16_t item_num);
static void M_Draw(const ITEM *item);

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = SKATE_KID_HITPOINTS;
    obj->radius = SKATE_KID_RADIUS;
    obj->smartness = SKATE_KID_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;

    Object_GetBone(obj, 0)->rot_y = true;

    if (!Object_Get(O_SKATEBOARD)->loaded) {
        LOG_WARNING(
            "Skateboard object (%d) is not loaded and so will not be drawn.",
            O_SKATEBOARD);
    }
}

static void M_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    Item_Get(item_num)->current_anim_state = SKATE_KID_STATE_SKATE;
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

    CREATURE *kid = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != SKATE_KID_STATE_DEATH) {
            item->current_anim_state = SKATE_KID_STATE_DEATH;
            Item_SwitchToAnim(item, SKATE_KID_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, SKATE_KID_SKATE_TURN);

        if (item->hit_points < SKATE_KID_SPEECH_HITPOINTS
            && !(item->flags & SKATE_KID_SPEECH_STARTED)) {
            Music_Play(MX_SKATEKID_SPEECH, MPM_TRACKED);
            item->flags |= SKATE_KID_SPEECH_STARTED;
        }

        switch (item->current_anim_state) {
        case SKATE_KID_STATE_STOP:
            kid->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = SKATE_KID_STATE_SHOOT_1;
            } else {
                item->goal_anim_state = SKATE_KID_STATE_SKATE;
            }
            break;

        case SKATE_KID_STATE_SKATE:
            kid->flags = 0;
            if (Random_GetControl() < SKATE_KID_PUSH_CHANCE) {
                item->goal_anim_state = SKATE_KID_STATE_PUSH;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance > SKATE_KID_DONT_STOP_RANGE
                    && info.distance < SKATE_KID_STOP_RANGE
                    && kid->mood != MOOD_ESCAPE) {
                    item->goal_anim_state = SKATE_KID_STATE_STOP;
                } else {
                    item->goal_anim_state = SKATE_KID_STATE_SHOOT_2;
                }
            }
            break;

        case SKATE_KID_STATE_PUSH:
            if (Random_GetControl() < SKATE_KID_SKATE_CHANCE) {
                item->goal_anim_state = SKATE_KID_STATE_SKATE;
            }
            break;

        case SKATE_KID_STATE_SHOOT_1:
        case SKATE_KID_STATE_SHOOT_2:
            if (!kid->flags && Creature_CanTargetEnemy(item, &info)) {
                Creature_ShootAtLara(
                    item, info.distance, &m_KidGun1, head,
                    item->current_anim_state == SKATE_KID_STATE_SHOOT_1
                        ? SKATE_KID_STOP_SHOT_DAMAGE
                        : SKATE_KID_SKATE_SHOT_DAMAGE);

                Creature_ShootAtLara(
                    item, info.distance, &m_KidGun2, head,
                    item->current_anim_state == SKATE_KID_STATE_SHOOT_1
                        ? SKATE_KID_STOP_SHOT_DAMAGE
                        : SKATE_KID_SKATE_SHOT_DAMAGE);

                kid->flags = 1;
            }
            if (kid->mood == MOOD_ESCAPE
                || info.distance < SKATE_KID_TOO_CLOSE) {
                item->required_anim_state = SKATE_KID_STATE_SKATE;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

static void M_Draw(const ITEM *const item)
{
    Object_DrawAnimatingItem(item);
    if (!Object_Get(O_SKATEBOARD)->loaded) {
        return;
    }

    const int16_t relative_anim = Item_GetRelativeAnim(item);
    const int16_t relative_frame = Item_GetRelativeFrame(item);
    ((ITEM *)item)->object_id = O_SKATEBOARD;
    Item_SwitchToAnim((ITEM *)item, relative_anim, relative_frame);
    Object_DrawAnimatingItem(item);

    ((ITEM *)item)->object_id = O_SKATEKID;
    Item_SwitchToAnim((ITEM *)item, relative_anim, relative_frame);
}

REGISTER_OBJECT(O_SKATEKID, M_Setup)
