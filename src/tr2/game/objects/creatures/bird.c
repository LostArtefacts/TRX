#include "game/objects/creatures/bird.h"

#include "game/creature.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define BIRD_DAMAGE 20
#define BIRD_RADIUS (WALL_L / 5) // = 204
#define BIRD_ATTACK_RANGE SQUARE(WALL_L / 2) // = 262144
#define BIRD_TURN (DEG_1 * 3) // = 546
#define BIRD_START_ANIM 5
#define BIRD_DIE_ANIM 8
#define EAGLE_HITPOINTS 20
#define CROW_HITPOINTS 15
#define CROW_START_ANIM 14
#define CROW_DIE_ANIM 1

typedef enum {
    BIRD_STATE_EMPTY = 0,
    BIRD_STATE_FLY = 1,
    BIRD_STATE_STOP = 2,
    BIRD_STATE_GLIDE = 3,
    BIRD_STATE_FALL = 4,
    BIRD_STATE_DEATH = 5,
    BIRD_STATE_ATTACK = 6,
    BIRD_STATE_EAT = 7,
} BIRD_STATE;

static const BITE m_BirdBite = {
    .pos = { .x = 15, .y = 46, .z = 21 },
    .mesh_num = 6,
};
static const BITE m_CrowBite = {
    .pos = { .x = 2, .y = 10, .z = 60 },
    .mesh_num = 14,
};

void Bird_SetupEagle(void)
{
    OBJECT *const obj = Object_Get(O_EAGLE);
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = Bird_Initialise;
    obj->control_func = Bird_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = EAGLE_HITPOINTS;
    obj->radius = BIRD_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Bird_SetupCrow(void)
{
    OBJECT *const obj = Object_Get(O_CROW);
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = Bird_Initialise;
    obj->control_func = Bird_Control;
    obj->collision_func = Creature_Collision;

    obj->hit_points = CROW_HITPOINTS;
    obj->radius = BIRD_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Bird_Initialise(const int16_t item_num)
{
    Creature_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_CROW) {
        Item_SwitchToAnim(item, CROW_START_ANIM, 0);
        item->goal_anim_state = BIRD_STATE_EAT;
        item->current_anim_state = BIRD_STATE_EAT;
    } else {
        Item_SwitchToAnim(item, BIRD_START_ANIM, 0);
        item->goal_anim_state = BIRD_STATE_STOP;
        item->current_anim_state = BIRD_STATE_STOP;
    }
}

void Bird_Control(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const bird = (CREATURE *)item->data;

    if (item->hit_points <= 0) {
        switch (item->current_anim_state) {
        case BIRD_STATE_FALL:
            if (item->pos.y > item->floor) {
                item->pos.y = item->floor;
                item->gravity = 0;
                item->fall_speed = 0;
                item->goal_anim_state = BIRD_STATE_DEATH;
            }
            break;

        case BIRD_STATE_DEATH:
            item->pos.y = item->floor;
            break;

        default:
            const int16_t anim_idx =
                item->object_id == O_CROW ? CROW_DIE_ANIM : BIRD_DIE_ANIM;
            Item_SwitchToAnim(item, anim_idx, 0);
            item->current_anim_state = BIRD_STATE_FALL;
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
    case BIRD_STATE_FLY:
        bird->flags = 0;
        if (item->required_anim_state != BIRD_STATE_EMPTY) {
            item->goal_anim_state = item->required_anim_state;
        }
        if (bird->mood == MOOD_BORED) {
            item->goal_anim_state = BIRD_STATE_STOP;
        } else if (info.ahead && info.distance < BIRD_ATTACK_RANGE) {
            item->goal_anim_state = BIRD_STATE_ATTACK;
        } else {
            item->goal_anim_state = BIRD_STATE_GLIDE;
        }
        break;

    case BIRD_STATE_STOP:
        item->pos.y = item->floor;
        if (bird->mood != MOOD_BORED) {
            item->goal_anim_state = BIRD_STATE_FLY;
        }
        break;

    case BIRD_STATE_GLIDE:
        if (bird->mood == MOOD_BORED) {
            item->required_anim_state = BIRD_STATE_STOP;
            item->goal_anim_state = BIRD_STATE_FLY;
        } else if (info.ahead && info.distance < BIRD_ATTACK_RANGE) {
            item->goal_anim_state = BIRD_STATE_ATTACK;
        }
        break;

    case BIRD_STATE_ATTACK:
        if (!bird->flags && item->touch_bits) {
            g_LaraItem->hit_points -= BIRD_DAMAGE;
            g_LaraItem->hit_status = 1;
            if (item->object_id == O_CROW) {
                Creature_Effect(item, &m_CrowBite, Spawn_Blood);
            } else {
                Creature_Effect(item, &m_BirdBite, Spawn_Blood);
            }
            bird->flags = 1;
        }
        break;

    case BIRD_STATE_EAT:
        item->pos.y = item->floor;
        if (bird->mood != MOOD_BORED) {
            item->goal_anim_state = BIRD_STATE_FLY;
        }
        break;
    }

    Creature_Animate(item_num, angle, 0);
}
