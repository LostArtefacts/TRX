#include "game/objects/creatures/bird.h"

#include "game/creature.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/utils.h>

typedef enum {
    BIRD_ANIM_EMPTY = 0,
    BIRD_ANIM_FLY = 1,
    BIRD_ANIM_STOP = 2,
    BIRD_ANIM_GLIDE = 3,
    BIRD_ANIM_FALL = 4,
    BIRD_ANIM_DEATH = 5,
    BIRD_ANIM_ATTACK = 6,
    BIRD_ANIM_EAT = 7,
} BIRD_ANIM;

static const BITE m_BirdBite = {
    .pos = { .x = 15, .y = 46, .z = 21 },
    .mesh_num = 6,
};
static const BITE m_CrowBite = {
    .pos = { .x = 2, .y = 10, .z = 60 },
    .mesh_num = 14,
};

#define BIRD_DAMAGE 20
#define BIRD_ATTACK_RANGE SQUARE(WALL_L / 2) // = 262144
#define BIRD_TURN (PHD_DEGREE * 3) // = 546
#define BIRD_START_ANIM 5
#define BIRD_DIE_ANIM 8
#define CROW_START_ANIM 14
#define CROW_DIE_ANIM 1

void __cdecl Bird_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = &g_Items[item_num];
    if (item->object_id == O_CROW) {
        item->anim_num = g_Objects[O_CROW].anim_idx + CROW_START_ANIM;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->goal_anim_state = BIRD_ANIM_EAT;
        item->current_anim_state = BIRD_ANIM_EAT;
    } else {
        item->anim_num = g_Objects[O_EAGLE].anim_idx + BIRD_START_ANIM;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->goal_anim_state = BIRD_ANIM_STOP;
        item->current_anim_state = BIRD_ANIM_STOP;
    }
}

void __cdecl Bird_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = &g_Items[item_num];
    CREATURE *const bird = (CREATURE *)item->data;

    if (item->hit_points <= 0) {
        switch (item->current_anim_state) {
        case BIRD_ANIM_FALL:
            if (item->pos.y > item->floor) {
                item->pos.y = item->floor;
                item->gravity = 0;
                item->fall_speed = 0;
                item->goal_anim_state = BIRD_ANIM_DEATH;
            }
            break;

        case BIRD_ANIM_DEATH:
            item->pos.y = item->floor;
            break;

        default:
            if (item->object_id == O_CROW) {
                item->anim_num = g_Objects[O_CROW].anim_idx + CROW_DIE_ANIM;
            } else {
                item->anim_num = g_Objects[O_EAGLE].anim_idx + BIRD_DIE_ANIM;
            }
            item->frame_num = g_Anims[item->anim_num].frame_base;
            item->current_anim_state = BIRD_ANIM_FALL;
            item->gravity = 1;
            item->speed = 0;
            break;
        }
        item->rot.x = 0;
        Creature_Animate(item_num, 0, 0);
        return;
    }

    AI_INFO info;
    Creature_AIInfo(item, &info);
    Creature_Mood(item, &info, MOOD_BORED);
    const int16_t angle = Creature_Turn(item, BIRD_TURN);

    switch (item->current_anim_state) {
    case BIRD_ANIM_FLY:
        bird->flags = 0;
        if (item->required_anim_state != BIRD_ANIM_EMPTY) {
            item->goal_anim_state = item->required_anim_state;
        }
        if (bird->mood == MOOD_BORED) {
            item->goal_anim_state = BIRD_ANIM_STOP;
        } else if (info.ahead && info.distance < BIRD_ATTACK_RANGE) {
            item->goal_anim_state = BIRD_ANIM_ATTACK;
        } else {
            item->goal_anim_state = BIRD_ANIM_GLIDE;
        }
        break;

    case BIRD_ANIM_STOP:
        item->pos.y = item->floor;
        if (bird->mood != MOOD_BORED) {
            item->goal_anim_state = BIRD_ANIM_FLY;
        }
        break;

    case BIRD_ANIM_GLIDE:
        if (bird->mood == MOOD_BORED) {
            item->required_anim_state = BIRD_ANIM_STOP;
            item->goal_anim_state = BIRD_ANIM_FLY;
        } else if (info.ahead && info.distance < BIRD_ATTACK_RANGE) {
            item->goal_anim_state = BIRD_ANIM_ATTACK;
        }
        break;

    case BIRD_ANIM_ATTACK:
        if (!bird->flags && item->touch_bits) {
            g_LaraItem->hit_points -= BIRD_DAMAGE;
            g_LaraItem->hit_status = 1;
            if (item->object_id == O_CROW) {
                Creature_Effect(item, &m_CrowBite, DoBloodSplat);
            } else {
                Creature_Effect(item, &m_BirdBite, DoBloodSplat);
            }
            bird->flags = 1;
        }
        break;

    case BIRD_ANIM_EAT:
        item->pos.y = item->floor;
        if (bird->mood != MOOD_BORED) {
            item->goal_anim_state = BIRD_ANIM_FLY;
        }
        break;
    }

    Creature_Animate(item_num, angle, 0);
}
