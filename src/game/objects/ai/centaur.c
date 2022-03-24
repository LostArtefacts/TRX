#include "game/objects/ai/centaur.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/effects/exploding_death.h"
#include "game/effects/gun.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

BITE_INFO g_CentaurRocket = { 11, 415, 41, 13 };
BITE_INFO g_CentaurRear = { 50, 30, 0, 5 };

void SetupCentaur(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = CentaurControl;
    obj->collision = CreatureCollision;
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
    g_AnimBones[obj->bone_index + 40] |= 0xCu;
}

void CentaurControl(int16_t item_num)
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
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, CENTAUR_TURN);

        switch (item->current_anim_state) {
        case CENTAUR_STOP:
            centaur->neck_rotation = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->goal_anim_state = CENTAUR_RUN;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = CENTAUR_AIM;
            } else {
                item->goal_anim_state = CENTAUR_RUN;
            }
            break;

        case CENTAUR_RUN:
            if (info.bite && info.distance < CENTAUR_REAR_RANGE) {
                item->required_anim_state = CENTAUR_WARNING;
                item->goal_anim_state = CENTAUR_STOP;
            } else if (Targetable(item, &info)) {
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
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = CENTAUR_SHOOT;
            } else {
                item->goal_anim_state = CENTAUR_STOP;
            }
            break;

        case CENTAUR_SHOOT:
            if (item->required_anim_state == CENTAUR_EMPTY) {
                item->required_anim_state = CENTAUR_AIM;
                int16_t fx_num =
                    CreatureEffect(item, &g_CentaurRocket, Effect_RocketGun);
                if (fx_num != NO_ITEM) {
                    centaur->neck_rotation = g_Effects[fx_num].pos.x_rot;
                }
            }
            break;

        case CENTAUR_WARNING:
            if (item->required_anim_state == CENTAUR_EMPTY
                && (item->touch_bits & CENTAUR_TOUCH)) {
                CreatureEffect(item, &g_CentaurRear, Effect_Blood);
                g_LaraItem->hit_points -= CENTAUR_REAR_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->required_anim_state = CENTAUR_STOP;
            }
            break;
        }
    }

    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);

    if (item->status == IS_DEACTIVATED) {
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        Effect_ExplodingDeath(item_num, -1, CENTAUR_PART_DAMAGE);
        KillItem(item_num);
        item->status = IS_DEACTIVATED;
    }
}
