#include "game/objects/creatures/skate_kid.h"

#include "game/creature.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <stdbool.h>

#define SKATE_KID_STOP_SHOT_DAMAGE 50
#define SKATE_KID_SKATE_SHOT_DAMAGE 40
#define SKATE_KID_STOP_RANGE SQUARE(WALL_L * 4) // = 16777216
#define SKATE_KID_DONT_STOP_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define SKATE_KID_TOO_CLOSE SQUARE(WALL_L) // = 1048576
#define SKATE_KID_SKATE_TURN (PHD_DEGREE * 4) // = 728
#define SKATE_KID_PUSH_CHANCE 0x200
#define SKATE_KID_SKATE_CHANCE 0x400
#define SKATE_KID_DIE_ANIM 13
#define SKATE_KID_HITPOINTS 125
#define SKATE_KID_RADIUS (WALL_L / 5) // = 204
#define SKATE_KID_SMARTNESS 0x7FFF
#define SKATE_KID_SPEECH_HITPOINTS 120
#define SKATE_KID_SPEECH_STARTED 1

typedef enum {
    SKATE_KID_STOP = 0,
    SKATE_KID_SHOOT = 1,
    SKATE_KID_SKATE = 2,
    SKATE_KID_PUSH = 3,
    SKATE_KID_SHOOT2 = 4,
    SKATE_KID_DEATH = 5,
} SKATE_KID_ANIM;

static BITE_INFO m_KidGun1 = { 0, 150, 34, 7 };
static BITE_INFO m_KidGun2 = { 0, 150, 37, 4 };

void SkateKid_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = SkateKid_Initialise;
    obj->control = SkateKid_Control;
    obj->draw_routine = SkateKid_Draw;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = SKATE_KID_HITPOINTS;
    obj->radius = SKATE_KID_RADIUS;
    obj->smartness = SKATE_KID_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index] |= BEB_ROT_Y;
}

void SkateKid_Initialise(int16_t item_num)
{
    Creature_Initialise(item_num);
    g_Items[item_num].current_anim_state = SKATE_KID_SKATE;
}

void SkateKid_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *kid = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != SKATE_KID_DEATH) {
            item->current_anim_state = SKATE_KID_DEATH;
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
            Music_Play(MX_SKATEKID_SPEECH);
            item->flags |= SKATE_KID_SPEECH_STARTED;
        }

        switch (item->current_anim_state) {
        case SKATE_KID_STOP:
            kid->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = SKATE_KID_SHOOT;
            } else {
                item->goal_anim_state = SKATE_KID_SKATE;
            }
            break;

        case SKATE_KID_SKATE:
            kid->flags = 0;
            if (Random_GetControl() < SKATE_KID_PUSH_CHANCE) {
                item->goal_anim_state = SKATE_KID_PUSH;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                if (info.distance > SKATE_KID_DONT_STOP_RANGE
                    && info.distance < SKATE_KID_STOP_RANGE
                    && kid->mood != MOOD_ESCAPE) {
                    item->goal_anim_state = SKATE_KID_STOP;
                } else {
                    item->goal_anim_state = SKATE_KID_SHOOT2;
                }
            }
            break;

        case SKATE_KID_PUSH:
            if (Random_GetControl() < SKATE_KID_SKATE_CHANCE) {
                item->goal_anim_state = SKATE_KID_SKATE;
            }
            break;

        case SKATE_KID_SHOOT:
        case SKATE_KID_SHOOT2:
            if (!kid->flags && Creature_CanTargetEnemy(item, &info)) {
                Creature_ShootAtLara(
                    item, info.distance, &m_KidGun1, head,
                    item->current_anim_state == SKATE_KID_SHOOT
                        ? SKATE_KID_STOP_SHOT_DAMAGE
                        : SKATE_KID_SKATE_SHOT_DAMAGE);

                Creature_ShootAtLara(
                    item, info.distance, &m_KidGun2, head,
                    item->current_anim_state == SKATE_KID_SHOOT
                        ? SKATE_KID_STOP_SHOT_DAMAGE
                        : SKATE_KID_SKATE_SHOT_DAMAGE);

                kid->flags = 1;
            }
            if (kid->mood == MOOD_ESCAPE
                || info.distance < SKATE_KID_TOO_CLOSE) {
                item->required_anim_state = SKATE_KID_SKATE;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}

void SkateKid_Draw(ITEM_INFO *item)
{
    Object_DrawAnimatingItem(item);

    int16_t relative_anim =
        item->anim_number - g_Objects[item->object_number].anim_index;
    int16_t relative_frame =
        item->frame_number - g_Anims[item->anim_number].frame_base;
    item->object_number = O_SKATEBOARD;
    Item_SwitchToAnim(item, relative_anim, relative_frame);
    Object_DrawAnimatingItem(item);

    item->object_number = O_SKATEKID;
    Item_SwitchToAnim(item, relative_anim, relative_frame);
}
