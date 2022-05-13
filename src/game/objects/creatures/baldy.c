#include "game/objects/creatures/baldy.h"

#include "game/collide.h"
#include "game/creature.h"
#include "game/items.h"
#include "game/los.h"
#include "game/lot.h"
#include "global/vars.h"

#define BALDY_SHOT_DAMAGE 150
#define BALDY_WALK_TURN (PHD_DEGREE * 3) // = 546
#define BALDY_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define BALDY_WALK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define BALDY_DIE_ANIM 14
#define BALDY_HITPOINTS 200
#define BALDY_RADIUS (WALL_L / 10) // = 102
#define BALDY_SMARTNESS 0x7FFF

typedef enum {
    BALDY_EMPTY = 0,
    BALDY_STOP = 1,
    BALDY_WALK = 2,
    BALDY_RUN = 3,
    BALDY_AIM = 4,
    BALDY_DEATH = 5,
    BALDY_SHOOT = 6,
} BALDY_ANIM;

static BITE_INFO m_BaldyGun = { -20, 440, 20, 9 };

void Baldy_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Baldy_Initialise;
    obj->control = Baldy_Control;
    obj->collision = Creature_Collision;
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
    Creature_Initialise(item_num);
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
            Item_Spawn(item, O_SHOTGUN_ITEM);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle = Creature_Turn(item, baldy->maximum_turn);

        switch (item->current_anim_state) {
        case BALDY_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_IsTargetable(item, &info)) {
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
            } else if (Creature_IsTargetable(item, &info)) {
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
                if (Creature_IsTargetable(item, &info)) {
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
            } else if (Creature_IsTargetable(item, &info)) {
                item->goal_anim_state = BALDY_SHOOT;
            } else {
                item->goal_anim_state = BALDY_STOP;
            }
            break;

        case BALDY_SHOOT:
            if (!baldy->flags) {
                Creature_ShootAtLara(
                    item, info.distance / 2, &m_BaldyGun, head,
                    BALDY_SHOT_DAMAGE);
                baldy->flags = 1;
            }
            if (baldy->mood == MOOD_ESCAPE) {
                item->required_anim_state = BALDY_RUN;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}
