#include "game/objects/creatures/cowboy.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/effects/gunshot.h"
#include "game/items.h"
#include "game/lot.h"
#include "global/vars.h"

#define COWBOY_SHOT_DAMAGE 70
#define COWBOY_WALK_TURN (PHD_DEGREE * 3) // = 546
#define COWBOY_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define COWBOY_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define COWBOY_DIE_ANIM 7
#define COWBOY_HITPOINTS 150
#define COWBOY_RADIUS (WALL_L / 10) // = 102
#define COWBOY_SMARTNESS 0x7FFF

typedef enum {
    COWBOY_EMPTY = 0,
    COWBOY_STOP = 1,
    COWBOY_WALK = 2,
    COWBOY_RUN = 3,
    COWBOY_AIM = 4,
    COWBOY_DEATH = 5,
    COWBOY_SHOOT = 6,
} COWBOY_ANIM;

static BITE_INFO m_CowboyGun1 = { 1, 200, 41, 5 };
static BITE_INFO m_CowboyGun2 = { -2, 200, 40, 8 };

void Cowboy_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Cowboy_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = COWBOY_HITPOINTS;
    obj->radius = COWBOY_RADIUS;
    obj->smartness = COWBOY_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index] |= BEB_ROT_Y;
}

void Cowboy_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

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
                g_Objects[O_COWBOY].anim_index + COWBOY_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            Item_Spawn(item, O_MAGNUM_ITEM);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, cowboy->maximum_turn);

        switch (item->current_anim_state) {
        case COWBOY_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_IsTargetable(item, &info)) {
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
            } else if (Creature_IsTargetable(item, &info)) {
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
                if (Creature_IsTargetable(item, &info)) {
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
            } else if (Creature_IsTargetable(item, &info)) {
                item->goal_anim_state = COWBOY_SHOOT;
            } else {
                item->goal_anim_state = COWBOY_STOP;
            }
            break;

        case COWBOY_SHOOT:
            if (!cowboy->flags) {
                if (Creature_ShootAtLara(
                        item, info.distance, &m_CowboyGun1, head)) {
                    g_LaraItem->hit_points -= COWBOY_SHOT_DAMAGE;
                    g_LaraItem->hit_status = 1;
                }
            } else if (cowboy->flags == 6) {
                if (Creature_IsTargetable(item, &info)) {
                    if (Creature_ShootAtLara(
                            item, info.distance, &m_CowboyGun2, head)) {
                        g_LaraItem->hit_points -= COWBOY_SHOT_DAMAGE;
                        g_LaraItem->hit_status = 1;
                    }
                } else {
                    int16_t fx_num =
                        Creature_Effect(item, &m_CowboyGun2, Effect_GunShot);
                    if (fx_num != NO_ITEM) {
                        g_Effects[fx_num].pos.y_rot += head;
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

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);
}
