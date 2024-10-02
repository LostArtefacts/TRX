#include "game/objects/creatures/torso.h"

#include "game/creature.h"
#include "game/effects/exploding_death.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lot.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <libtrx/utils.h>

#define EXTRA_ANIM_TORSO_SLAM 0
#define TORSO_TURN_L_ANIM 8
#define TORSO_DIE_ANIM 13
#define TORSO_TURN_R_ANIM 17
#define TORSO_PART_DAMAGE 250
#define TORSO_ATTACK_DAMAGE 500
#define TORSO_TOUCH_DAMAGE 5
#define TORSO_NEED_TURN (PHD_DEGREE * 45) // = 8190
#define TORSO_TURN (PHD_DEGREE * 3) // = 546
#define TORSO_ATTACK_RANGE SQUARE(2600) // = 6760000
#define TORSO_CLOSE_RANGE SQUARE(2250) // = 5062500
#define TORSO_TLEFT 0x7FF0
#define TORSO_TRIGHT 0x3FF8000
#define TORSO_TOUCH (TORSO_TLEFT | TORSO_TRIGHT)
#define TORSO_HITPOINTS 500
#define TORSO_RADIUS (WALL_L / 3) // = 341
#define TORSO_SMARTNESS 0x7FFF
#define TORSO_FRAME_TURN_L_START 14
#define TORSO_FRAME_TURN_L_END 22
#define TORSO_FRAME_TURN_R_START 17
#define TORSO_FRAME_TURN_R_END 22

typedef enum {
    TORSO_EMPTY = 0,
    TORSO_STOP = 1,
    TORSO_TURN_L = 2,
    TORSO_TURN_R = 3,
    TORSO_ATTACK1 = 4,
    TORSO_ATTACK2 = 5,
    TORSO_ATTACK3 = 6,
    TORSO_FORWARD = 7,
    TORSO_SET = 8,
    TORSO_FALL = 9,
    TORSO_DEATH = 10,
    TORSO_KILL = 11,
} TORSO_ANIM;

void Torso_Setup(OBJECT *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Creature_Initialise;
    obj->control = Torso_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = TORSO_HITPOINTS;
    obj->radius = TORSO_RADIUS;
    obj->smartness = TORSO_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_idx + 4] |= BEB_ROT_Y;
}

void Torso_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *torso = item->data;
    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != TORSO_DEATH) {
            item->current_anim_state = TORSO_DEATH;
            Item_SwitchToAnim(item, TORSO_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        angle =
            Math_Atan(
                torso->target.z - item->pos.z, torso->target.x - item->pos.x)
            - item->rot.y;

        if (item->touch_bits) {
            Lara_TakeDamage(TORSO_TOUCH_DAMAGE, true);
        }

        switch (item->current_anim_state) {
        case TORSO_SET:
            item->goal_anim_state = TORSO_FALL;
            item->gravity = 1;
            break;

        case TORSO_STOP:
            if (g_LaraItem->hit_points <= 0) {
                break;
            }

            torso->flags = 0;
            if (angle > TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_TURN_R;
            } else if (angle < -TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_TURN_L;
            } else if (info.distance >= TORSO_ATTACK_RANGE) {
                item->goal_anim_state = TORSO_FORWARD;
            } else if (g_LaraItem->hit_points > TORSO_ATTACK_DAMAGE) {
                if (Random_GetControl() < 0x4000) {
                    item->goal_anim_state = TORSO_ATTACK1;
                } else {
                    item->goal_anim_state = TORSO_ATTACK2;
                }
            } else if (info.distance < TORSO_CLOSE_RANGE) {
                item->goal_anim_state = TORSO_ATTACK3;
            } else {
                item->goal_anim_state = TORSO_FORWARD;
            }
            break;

        case TORSO_FORWARD:
            if (angle < -TORSO_TURN) {
                item->goal_anim_state -= TORSO_TURN;
            } else if (angle > TORSO_TURN) {
                item->goal_anim_state += TORSO_TURN;
            } else {
                item->goal_anim_state += angle;
            }

            if (angle > TORSO_NEED_TURN || angle < -TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STOP;
            } else if (info.distance < TORSO_ATTACK_RANGE) {
                item->goal_anim_state = TORSO_STOP;
            }
            break;

        case TORSO_TURN_L:
            if (!torso->flags) {
                torso->flags = item->frame_num;
            } else if (
                Item_TestAnimEqual(item, TORSO_TURN_L_ANIM)
                && Item_TestFrameRange(
                    item, TORSO_FRAME_TURN_L_START, TORSO_FRAME_TURN_L_END)) {
                item->rot.y -= PHD_DEGREE * 9;
            }

            if (angle > -TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STOP;
            }
            break;

        case TORSO_TURN_R:
            if (!torso->flags) {
                torso->flags = item->frame_num;
            } else if (
                Item_TestAnimEqual(item, TORSO_TURN_R_ANIM)
                && Item_TestFrameRange(
                    item, TORSO_FRAME_TURN_R_START, TORSO_FRAME_TURN_R_END)) {
                item->rot.y += PHD_DEGREE * 14;
            }

            if (angle < TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STOP;
            }
            break;

        case TORSO_ATTACK1:
            if (!torso->flags && (item->touch_bits & TORSO_TRIGHT)) {
                Lara_TakeDamage(TORSO_ATTACK_DAMAGE, true);
                torso->flags = 1;
            }
            break;

        case TORSO_ATTACK2:
            if (!torso->flags && (item->touch_bits & TORSO_TOUCH)) {
                Lara_TakeDamage(TORSO_ATTACK_DAMAGE, true);
                torso->flags = 1;
            }
            break;

        case TORSO_ATTACK3:
            if ((item->touch_bits & TORSO_TRIGHT)
                || g_LaraItem->hit_points <= 0) {
                item->goal_anim_state = TORSO_KILL;

                Item_SwitchToObjAnim(
                    g_LaraItem, EXTRA_ANIM_TORSO_SLAM, 0, O_LARA_EXTRA);
                g_LaraItem->current_anim_state = LS_SPECIAL;
                g_LaraItem->goal_anim_state = LS_SPECIAL;
                g_LaraItem->room_num = item->room_num;
                g_LaraItem->pos.x = item->pos.x;
                g_LaraItem->pos.y = item->pos.y;
                g_LaraItem->pos.z = item->pos.z;
                g_LaraItem->rot.x = 0;
                g_LaraItem->rot.y = item->rot.y;
                g_LaraItem->rot.z = 0;
                g_LaraItem->gravity = 0;
                g_LaraItem->hit_points = -1;
                g_Lara.air = -1;
                g_Lara.gun_status = LGS_HANDS_BUSY;
                g_Lara.gun_type = LGT_UNARMED;

                g_Camera.target_distance = WALL_L * 2;
                g_Camera.flags = FOLLOW_CENTRE;
            }
            break;

        case TORSO_KILL:
            g_Camera.target_distance = WALL_L * 2;
            g_Camera.flags = FOLLOW_CENTRE;
            break;
        }
    }

    Creature_Head(item, head);

    if (item->current_anim_state == TORSO_FALL) {
        Item_Animate(item);

        if (item->pos.y > item->floor) {
            item->goal_anim_state = TORSO_STOP;
            item->gravity = 0;
            item->pos.y = item->floor;
            g_Camera.bounce = 500;
        }
    } else {
        Creature_Animate(item_num, 0, 0);
    }

    if (item->status == IS_DEACTIVATED) {
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        Effect_ExplodingDeath(item_num, -1, TORSO_PART_DAMAGE);
        Room_TestTriggers(item);

        Item_Kill(item_num);
        item->status = IS_DEACTIVATED;
    }
}
