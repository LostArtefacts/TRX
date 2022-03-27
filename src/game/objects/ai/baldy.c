#include "game/objects/ai/baldy.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "global/vars.h"

BITE_INFO g_BaldyGun = { -20, 440, 20, 9 };

void Baldy_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Baldy_Initialise;
    obj->control = Baldy_Control;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = BALDY_HITPOINTS;
    obj->radius = BALDY_RADIUS;
    obj->smartness = BALDY_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index] |= BEB_ROT_Y;
}

void Baldy_Initialise(int16_t item_num)
{
    InitialiseCreature(item_num);
    g_Items[item_num].current_anim_state = BALDY_RUN;
}

void Baldy_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *baldy = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != BALDY_DEATH) {
            item->current_anim_state = BALDY_DEATH;
            item->anim_number = g_Objects[O_BALDY].anim_index + BALDY_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            SpawnItem(item, O_SHOTGUN_ITEM);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, baldy->maximum_turn);

        switch (item->current_anim_state) {
        case BALDY_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = BALDY_AIM;
            } else if (baldy->mood == MOOD_BORED) {
                item->goal_anim_state = BALDY_WALK;
            } else {
                item->goal_anim_state = BALDY_RUN;
            }
            break;

        case BALDY_WALK:
            baldy->maximum_turn = BALDY_WALK_TURN;
            if (baldy->mood == MOOD_ESCAPE || !info.ahead) {
                item->required_anim_state = BALDY_RUN;
                item->goal_anim_state = BALDY_STOP;
            } else if (Targetable(item, &info)) {
                item->required_anim_state = BALDY_AIM;
                item->goal_anim_state = BALDY_STOP;
            } else if (info.distance > BALDY_WALK_RANGE) {
                item->required_anim_state = BALDY_RUN;
                item->goal_anim_state = BALDY_STOP;
            }
            break;

        case BALDY_RUN:
            baldy->maximum_turn = BALDY_RUN_TURN;
            tilt = angle / 2;
            if (baldy->mood != MOOD_ESCAPE || info.ahead) {
                if (Targetable(item, &info)) {
                    item->required_anim_state = BALDY_AIM;
                    item->goal_anim_state = BALDY_STOP;
                } else if (info.ahead && info.distance < BALDY_WALK_RANGE) {
                    item->required_anim_state = BALDY_WALK;
                    item->goal_anim_state = BALDY_STOP;
                }
            }
            break;

        case BALDY_AIM:
            baldy->flags = 0;
            if (item->required_anim_state) {
                item->goal_anim_state = BALDY_STOP;
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = BALDY_SHOOT;
            } else {
                item->goal_anim_state = BALDY_STOP;
            }
            break;

        case BALDY_SHOOT:
            if (!baldy->flags) {
                if (ShotLara(item, info.distance / 2, &g_BaldyGun, head)) {
                    g_LaraItem->hit_points -= BALDY_SHOT_DAMAGE;
                    g_LaraItem->hit_status = 1;
                }
                baldy->flags = 1;
            }
            if (baldy->mood == MOOD_ESCAPE) {
                item->required_anim_state = BALDY_RUN;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);
}
