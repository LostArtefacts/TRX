#include "game/objects/creatures/pierre.h"

#include "config.h"
#include "game/creature.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/los.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"

#include <stdbool.h>

#define PIERRE_POSE_CHANCE 0x60 // = 96
#define PIERRE_SHOT_DAMAGE 50
#define PIERRE_WALK_TURN (PHD_DEGREE * 3) // = 546
#define PIERRE_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define PIERRE_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define PIERRE_DIE_ANIM 12
#define PIERRE_WIMP_CHANCE 0x2000
#define PIERRE_RUN_HITPOINTS 40
#define PIERRE_DISAPPEAR 10
#define PIERRE_HITPOINTS 70
#define PIERRE_RADIUS (WALL_L / 10) // = 102
#define PIERRE_SMARTNESS 0x7FFF

typedef enum {
    PIERRE_EMPTY = 0,
    PIERRE_STOP = 1,
    PIERRE_WALK = 2,
    PIERRE_RUN = 3,
    PIERRE_AIM = 4,
    PIERRE_DEATH = 5,
    PIERRE_POSE = 6,
    PIERRE_SHOOT = 7,
} PIERRE_ANIM;

static BITE_INFO m_PierreGun1 = { 60, 200, 0, 11 };
static BITE_INFO m_PierreGun2 = { -57, 200, 0, 14 };
static int16_t m_PierreItemNum = NO_ITEM;

void Pierre_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Pierre_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = PIERRE_HITPOINTS;
    obj->radius = PIERRE_RADIUS;
    obj->smartness = PIERRE_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 24] |= BEB_ROT_Y;
}

void Pierre_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (g_Config.change_pierre_spawn) {
        if (m_PierreItemNum == NO_ITEM) {
            m_PierreItemNum = item_num;
        } else if (m_PierreItemNum != item_num) {
            ITEM_INFO *old_pierre = &g_Items[m_PierreItemNum];
            if (old_pierre->flags & IF_ONESHOT) {
                if (!(item->flags & IF_ONESHOT)) {
                    Item_Kill(item_num);
                }
            } else {
                Item_Kill(m_PierreItemNum);
                m_PierreItemNum = item_num;
            }
        }
    } else {
        if (m_PierreItemNum == NO_ITEM) {
            m_PierreItemNum = item_num;
        } else if (m_PierreItemNum != item_num) {
            if (item->flags & IF_ONESHOT) {
                Item_Kill(m_PierreItemNum);
            } else {
                Item_Kill(item_num);
            }
        }
    }

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *pierre = item->data;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t tilt = 0;

    if (item->hit_points <= PIERRE_RUN_HITPOINTS
        && !(item->flags & IF_ONESHOT)) {
        item->hit_points = PIERRE_RUN_HITPOINTS;
        pierre->flags++;
    }

    if (item->hit_points <= 0) {
        if (item->current_anim_state != PIERRE_DEATH) {
            item->current_anim_state = PIERRE_DEATH;
            Item_SwitchToAnim(item, PIERRE_DIE_ANIM, -1);
            if (Inv_RequestItem(O_MAGNUM_ITEM)) {
                Item_Spawn(item, O_MAG_AMMO_ITEM);
            } else {
                Item_Spawn(item, O_MAGNUM_ITEM);
            }
            Item_Spawn(item, O_SCION_ITEM2);
            Item_Spawn(item, O_KEY_ITEM1);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        if (pierre->flags) {
            info.enemy_zone = -1;
            item->hit_status = 1;
        }
        Creature_Mood(item, &info, false);

        angle = Creature_Turn(item, pierre->maximum_turn);

        switch (item->current_anim_state) {
        case PIERRE_STOP:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (pierre->mood == MOOD_BORED) {
                item->goal_anim_state = Random_GetControl() < PIERRE_POSE_CHANCE
                    ? PIERRE_POSE
                    : PIERRE_WALK;
            } else if (pierre->mood == MOOD_ESCAPE) {
                item->goal_anim_state = PIERRE_RUN;
            } else {
                item->goal_anim_state = PIERRE_WALK;
            }
            break;

        case PIERRE_POSE:
            if (pierre->mood != MOOD_BORED) {
                item->goal_anim_state = PIERRE_STOP;
            } else if (Random_GetControl() < PIERRE_POSE_CHANCE) {
                item->required_anim_state = PIERRE_WALK;
                item->goal_anim_state = PIERRE_STOP;
            }
            break;

        case PIERRE_WALK:
            pierre->maximum_turn = PIERRE_WALK_TURN;
            if (pierre->mood == MOOD_BORED
                && Random_GetControl() < PIERRE_POSE_CHANCE) {
                item->required_anim_state = PIERRE_POSE;
                item->goal_anim_state = PIERRE_STOP;
            } else if (pierre->mood == MOOD_ESCAPE) {
                item->required_anim_state = PIERRE_RUN;
                item->goal_anim_state = PIERRE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = PIERRE_AIM;
                item->goal_anim_state = PIERRE_STOP;
            } else if (!info.ahead || info.distance > PIERRE_WALK_RANGE) {
                item->required_anim_state = PIERRE_RUN;
                item->goal_anim_state = PIERRE_STOP;
            }
            break;

        case PIERRE_RUN:
            pierre->maximum_turn = PIERRE_RUN_TURN;
            tilt = angle / 2;
            if (pierre->mood == MOOD_BORED
                && Random_GetControl() < PIERRE_POSE_CHANCE) {
                item->required_anim_state = PIERRE_POSE;
                item->goal_anim_state = PIERRE_STOP;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->required_anim_state = PIERRE_AIM;
                item->goal_anim_state = PIERRE_STOP;
            } else if (info.ahead && info.distance < PIERRE_WALK_RANGE) {
                item->required_anim_state = PIERRE_WALK;
                item->goal_anim_state = PIERRE_STOP;
            }
            break;

        case PIERRE_AIM:
            if (item->required_anim_state) {
                item->goal_anim_state = item->required_anim_state;
            } else if (Creature_CanTargetEnemy(item, &info)) {
                item->goal_anim_state = PIERRE_SHOOT;
            } else {
                item->goal_anim_state = PIERRE_STOP;
            }
            break;

        case PIERRE_SHOOT:
            if (!item->required_anim_state) {
                Creature_ShootAtLara(
                    item, info.distance, &m_PierreGun1, head,
                    PIERRE_SHOT_DAMAGE / 2);
                Creature_ShootAtLara(
                    item, info.distance, &m_PierreGun2, head,
                    PIERRE_SHOT_DAMAGE / 2);
                item->required_anim_state = PIERRE_AIM;
            }
            if (pierre->mood == MOOD_ESCAPE
                && Random_GetControl() > PIERRE_WIMP_CHANCE) {
                item->required_anim_state = PIERRE_STOP;
            }
            break;
        }
    }

    Creature_Tilt(item, tilt);
    Creature_Head(item, head);
    Creature_Animate(item_num, angle, 0);

    if (pierre->flags) {
        GAME_VECTOR target;
        target.x = item->pos.x;
        target.y = item->pos.y - WALL_L;
        target.z = item->pos.z;

        GAME_VECTOR start;
        start.x = g_Camera.pos.x;
        start.y = g_Camera.pos.y;
        start.z = g_Camera.pos.z;
        start.room_number = g_Camera.pos.room_number;

        if (LOS_Check(&start, &target)) {
            pierre->flags = 1;
        } else if (pierre->flags > PIERRE_DISAPPEAR) {
            item->hit_points = DONT_TARGET;
            LOT_DisableBaddieAI(item_num);
            Item_Kill(item_num);
            m_PierreItemNum = NO_ITEM;
        }
    }

    int16_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT) {
        item->hit_points = DONT_TARGET;
        LOT_DisableBaddieAI(item_num);
        Item_Kill(item_num);
        m_PierreItemNum = NO_ITEM;
    }
}

void Pierre_Reset(void)
{
    m_PierreItemNum = NO_ITEM;
}
