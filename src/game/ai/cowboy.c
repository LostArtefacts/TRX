#include "game/ai/cowboy.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/vars.h"

BITE_INFO CowboyGun1 = { 1, 200, 41, 5 };
BITE_INFO CowboyGun2 = { -2, 200, 40, 8 };

void SetupCowboy(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = CowboyControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = COWBOY_HITPOINTS;
    obj->radius = COWBOY_RADIUS;
    obj->smartness = COWBOY_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    AnimBones[obj->bone_index] |= BEB_ROT_Y;
}

void CowboyControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *cowboy = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != COWBOY_DEATH) {
            item->current_anim_state = COWBOY_DEATH;
            item->anim_number =
                Objects[O_MERCENARY2].anim_index + COWBOY_DIE_ANIM;
            item->frame_number = Anims[item->anim_number].frame_base;
            SpawnItem(item, O_MAGNUM_ITEM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, cowboy->maximum_turn);

        switch (item->current_anim_state) {
        case COWBOY_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = COWBOY_AIM;
            } else if (cowboy->mood == MOOD_BORED) {
                item->goal_anim_state = COWBOY_WALK;
            } else {
                item->goal_anim_state = COWBOY_RUN;
            }
            break;

        case COWBOY_WALK:
            cowboy->maximum_turn = COWBOY_WALK_TURN;
            if (cowboy->mood == MOOD_ESCAPE || !info.ahead) {
                item->required_anim_state = COWBOY_RUN;
                item->goal_anim_state = COWBOY_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = COWBOY_AIM;
                item->goal_anim_state = COWBOY_STOP;
            } else if (info.distance > COWBOY_WALK_RANGE) {
                item->required_anim_state = COWBOY_RUN;
                item->goal_anim_state = COWBOY_STOP;
            }
            break;

        case COWBOY_RUN:
            cowboy->maximum_turn = COWBOY_RUN_TURN;
            tilt = angle / 2;
            if (cowboy->mood != MOOD_ESCAPE || info.ahead) {
                if (Targetable(item, &info)) {
                    item->required_anim_state = COWBOY_AIM;
                    item->goal_anim_state = COWBOY_STOP;
                } else if (info.ahead && info.distance < COWBOY_WALK_RANGE) {
                    item->required_anim_state = COWBOY_WALK;
                    item->goal_anim_state = COWBOY_STOP;
                }
            }
            break;

        case COWBOY_AIM:
            cowboy->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = COWBOY_STOP;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = COWBOY_SHOOT;
            } else {
                item->goal_anim_state = COWBOY_STOP;
            }
            break;

        case COWBOY_SHOOT:
            if (!cowboy->flags) {
                if (ShotLara(item, info.distance, &CowboyGun1, head)) {
                    LaraItem->hit_points -= COWBOY_SHOT_DAMAGE;
                    LaraItem->hit_status = 1;
                }
            } else if (cowboy->flags == 6) {
                if (Targetable(item, &info)) {
                    if (ShotLara(item, info.distance, &CowboyGun2, head)) {
                        LaraItem->hit_points -= COWBOY_SHOT_DAMAGE;
                        LaraItem->hit_status = 1;
                    }
                } else {
                    int16_t fx_num = CreatureEffect(item, &CowboyGun2, GunShot);
                    if (fx_num != NO_ITEM) {
                        Effects[fx_num].pos.y_rot += head;
                    }
                }
            }
            cowboy->flags++;

            if (cowboy->mood == MOOD_ESCAPE) {
                item->required_anim_state = COWBOY_RUN;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);
}
