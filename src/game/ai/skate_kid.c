#include "game/ai/skate_kid.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/people.h"
#include "game/random.h"
#include "global/vars.h"

BITE_INFO g_KidGun1 = { 0, 150, 34, 7 };
BITE_INFO g_KidGun2 = { 0, 150, 37, 4 };

void SetupSkateKid(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseSkateKid;
    obj->control = SkateKidControl;
    obj->draw_routine = DrawSkateKid;
    obj->collision = CreatureCollision;
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

void InitialiseSkateKid(int16_t item_num)
{
    InitialiseCreature(item_num);
    g_Items[item_num].current_anim_state = SKATE_KID_SKATE;
}

void SkateKidControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    CREATURE_INFO *kid = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != SKATE_KID_DEATH) {
            item->current_anim_state = SKATE_KID_DEATH;
            item->anim_number =
                g_Objects[O_MERCENARY1].anim_index + SKATE_KID_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            SpawnItem(item, O_UZI_ITEM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, SKATE_KID_SKATE_TURN);

        if (item->hit_points < 120 && Music_CurrentTrack() != 56) {
            Music_Play(56);
        }

        switch (item->current_anim_state) {
        case SKATE_KID_STOP:
            kid->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = SKATE_KID_SHOOT;
            } else {
                item->goal_anim_state = SKATE_KID_SKATE;
            }
            break;

        case SKATE_KID_SKATE:
            kid->flags = 0;
            if (Random_GetControl() < SKATE_KID_PUSH_CHANCE) {
                item->goal_anim_state = SKATE_KID_PUSH;
            } else if (Targetable(item, &info)) {
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
            if (!kid->flags && Targetable(item, &info)) {
                if (ShotLara(item, info.distance, &g_KidGun1, head)) {
                    g_LaraItem->hit_points -=
                        item->current_anim_state == SKATE_KID_SHOOT
                        ? SKATE_KID_STOP_SHOT_DAMAGE
                        : SKATE_KID_SKATE_SHOT_DAMAGE;
                    g_LaraItem->hit_status = 1;
                }

                if (ShotLara(item, info.distance, &g_KidGun2, head)) {
                    g_LaraItem->hit_points -=
                        item->current_anim_state == SKATE_KID_SHOOT
                        ? SKATE_KID_STOP_SHOT_DAMAGE
                        : SKATE_KID_SKATE_SHOT_DAMAGE;
                    g_LaraItem->hit_status = 1;
                }

                kid->flags = 1;
            }
            if (kid->mood == MOOD_ESCAPE
                || info.distance < SKATE_KID_TOO_CLOSE) {
                item->required_anim_state = SKATE_KID_SKATE;
            }
            break;
        }
    }

    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);
}

void DrawSkateKid(ITEM_INFO *item)
{
    DrawAnimatingItem(item);
    int16_t anim = item->anim_number;
    int16_t frame = item->frame_number;
    item->object_number = O_SKATEBOARD;
    item->anim_number = anim + g_Objects[O_SKATEBOARD].anim_index
        - g_Objects[O_MERCENARY1].anim_index;
    DrawAnimatingItem(item);
    item->anim_number = anim;
    item->frame_number = frame;
    item->object_number = O_MERCENARY1;
}
