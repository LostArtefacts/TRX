#include "game/objects/creatures/centaur.h"

#include "game/collide.h"
#include "game/creature.h"
#include "game/effects/blood.h"
#include "game/effects/exploding_death.h"
#include "game/effects/gun.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

#define CENTAUR_PART_DAMAGE 100
#define CENTAUR_REAR_DAMAGE 200
#define CENTAUR_TOUCH 0x30199
#define CENTAUR_DIE_ANIM 8
#define CENTAUR_TURN (PHD_DEGREE * 4) // = 728
#define CENTAUR_REAR_CHANCE 96
#define CENTAUR_REAR_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define CENTAUR_HITPOINTS 120
#define CENTAUR_RADIUS (WALL_L / 3) // = 341
#define CENTAUR_SMARTNESS 0x7FFF

typedef enum {
    CENTAUR_EMPTY = 0,
    CENTAUR_STOP = 1,
    CENTAUR_SHOOT = 2,
    CENTAUR_RUN = 3,
    CENTAUR_AIM = 4,
    CENTAUR_DEATH = 5,
    CENTAUR_WARNING = 6,
} CENTAUR_ANIM;

static BITE_INFO m_CentaurRocket = { 11, 415, 41, 13 };
static BITE_INFO m_CentaurRear = { 50, 30, 0, 5 };

void Centaur_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Centaur_Control;
    obj->collision = Creature_Collision;
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
    g_AnimBones[obj->bone_index + 40] |= BEB_ROT_Z | BEB_POP;
}

void Centaur_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *centaur = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != CENTAUR_DEATH) {
            item->current_anim_state = CENTAUR_DEATH;
            item->anim_number =
                g_Objects[O_CENTAUR].anim_index + CENTAUR_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
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
        case CENTAUR_STOP:
            centaur->neck_rotation = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->goal_anim_state = CENTAUR_RUN;
            } else if (Creature_IsTargetable(item, &info)) {
                item->goal_anim_state = CENTAUR_AIM;
            } else {
                item->goal_anim_state = CENTAUR_RUN;
            }
            break;

        case CENTAUR_RUN:
            if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->required_anim_state = CENTAUR_WARNING;
                item->goal_anim_state = CENTAUR_STOP;
            } else if (Creature_IsTargetable(item, &info)) {
                item->required_anim_state = CENTAUR_AIM;
                item->goal_anim_state = CENTAUR_STOP;
            } else if (Random_GetControl() < CENTAUR_REAR_CHANCE) {
                item->required_anim_state = CENTAUR_WARNING;
                item->goal_anim_state = CENTAUR_STOP;
            }
            break;

        case CENTAUR_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_IsTargetable(item, &info)) {
                item->goal_anim_state = CENTAUR_SHOOT;
            } else {
                item->goal_anim_state = CENTAUR_STOP;
            }
            break;

        case CENTAUR_SHOOT:
            if (item->required_anim_state == CENTAUR_EMPTY) {
                item->required_anim_state = CENTAUR_AIM;
                int16_t fx_num =
                    Creature_Effect(item, &m_CentaurRocket, Effect_RocketGun);
                if (fx_num != NO_ITEM) {
                    centaur->neck_rotation = g_Effects[fx_num].pos.x_rot;
                }
            }
            break;

        case CENTAUR_WARNING:
            if (item->required_anim_state == CENTAUR_EMPTY
                && (item->touch_bits & CENTAUR_TOUCH)) {
                Creature_Effect(item, &m_CentaurRear, Effect_Blood);
                g_LaraItem->hit_points -= CENTAUR_REAR_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->required_anim_state = CENTAUR_STOP;
            }
            break;
        }
    }

    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);

    if (item->status == IS_DEACTIVATED) {
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        Effect_ExplodingDeath(item_num, -1, CENTAUR_PART_DAMAGE);
        Item_Kill(item_num);
        item->status = IS_DEACTIVATED;
    }
}
