#include "game/ai/skate_kid.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/vars.h"
#include "specific/sndpc.h"

BITE_INFO KidGun1 = { 0, 150, 34, 7 };
BITE_INFO KidGun2 = { 0, 150, 37, 4 };

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
    obj->hit_points = SKATEKID_HITPOINTS;
    obj->radius = SKATEKID_RADIUS;
    obj->smartness = SKATEKID_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index] |= BEB_ROT_Y;
}

void InitialiseSkateKid(int16_t item_num)
{
    InitialiseCreature(item_num);
    Items[item_num].current_anim_state = KID_SKATE;
}

void SkateKidControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    CREATURE_INFO *kid = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != KID_DEATH) {
            item->current_anim_state = KID_DEATH;
            item->anim_number = Objects[O_MERCENARY1].anim_index + KID_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            SpawnItem(item, O_UZI_ITEM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, KID_SKATE_TURN);

        if (item->hit_points < 120 && CDTrack != 56) {
            S_CDPlay(56);
        }

        switch (item->current_anim_state) {
        case KID_STOP:
            kid->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = KID_SHOOT;
            } else {
                item->goal_anim_state = KID_SKATE;
            }
            break;

        case KID_SKATE:
            kid->flags = 0;
            if (GetRandomControl() < KID_PUSH_CHANCE) {
                item->goal_anim_state = KID_PUSH;
            } else if (Targetable(item, &info)) {
                if (info.distance > KID_DONT_STOP_RANGE
                    && info.distance < KID_STOP_RANGE
                    && kid->mood != MOOD_ESCAPE) {
                    item->goal_anim_state = KID_STOP;
                } else {
                    item->goal_anim_state = KID_SHOOT2;
                }
            }
            break;

        case KID_PUSH:
            if (GetRandomControl() < KID_SKATE_CHANCE) {
                item->goal_anim_state = KID_SKATE;
            }
            break;

        case KID_SHOOT:
        case KID_SHOOT2:
            if (!kid->flags && Targetable(item, &info)) {
                if (ShotLara(item, info.distance, &KidGun1, head)) {
                    LaraItem->hit_points -=
                        item->current_anim_state == KID_SHOOT
                        ? KID_STOP_SHOT_DAMAGE
                        : KID_SKATE_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }

                if (ShotLara(item, info.distance, &KidGun2, head)) {
                    LaraItem->hit_points -=
                        item->current_anim_state == KID_SHOOT
                        ? KID_STOP_SHOT_DAMAGE
                        : KID_SKATE_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }

                kid->flags = 1;
            }
            if (kid->mood == MOOD_ESCAPE || info.distance < KID_TOO_CLOSE) {
                item->required_anim_state = KID_SKATE;
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
    item->anim_number = anim + Objects[O_SKATEBOARD].anim_index
        - Objects[O_MERCENARY1].anim_index;
    DrawAnimatingItem(item);
    item->anim_number = anim;
    item->frame_number = frame;
    item->object_number = O_MERCENARY1;
}
