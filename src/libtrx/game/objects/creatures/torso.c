#include "game/camera.h"
#include "game/creature.h"
#include "game/lara.h"
#include "game/math.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "utils.h"

#define EXTRA_ANIM_TORSO_SLAM 0
#define TORSO_TURN_L_ANIM 8
#define TORSO_DIE_ANIM 13
#define TORSO_TURN_R_ANIM 17
#define TORSO_PART_DAMAGE 250
#define TORSO_ATTACK_DAMAGE 500
#define TORSO_TOUCH_DAMAGE 5
#define TORSO_NEED_TURN (DEG_1 * 45) // = 8190
#define TORSO_TURN (DEG_1 * 3) // = 546
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
    TORSO_STATE_EMPTY = 0,
    TORSO_STATE_STOP = 1,
    TORSO_STATE_TURN_L = 2,
    TORSO_STATE_TURN_R = 3,
    TORSO_STATE_ATTACK_1 = 4,
    TORSO_STATE_ATTACK_2 = 5,
    TORSO_STATE_ATTACK_3 = 6,
    TORSO_STATE_FORWARD = 7,
    TORSO_STATE_SET = 8,
    TORSO_STATE_FALL = 9,
    TORSO_STATE_DEATH = 10,
    TORSO_STATE_KILL = 11,
} TORSO_STATE;

static void M_KillLara(const ITEM *const item)
{
    ITEM *const lara_item = Lara_GetItem();
    Item_SwitchToObjAnim(lara_item, EXTRA_ANIM_TORSO_SLAM, 0, O_LARA_EXTRA);

    lara_item->current_anim_state = LS(LS_SPECIAL);
    lara_item->goal_anim_state = LS(LS_SPECIAL);
    lara_item->room_num = item->room_num;
    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = item->pos.y;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.x = 0;
    lara_item->rot.y = item->rot.y;
    lara_item->rot.z = 0;
    lara_item->gravity = 0;
    lara_item->hit_points = -1;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->extra_anim = true;
    lara->air = -1;
    lara->gun_status = LGS_HANDS_BUSY;
    lara->gun_type = LGT_UNARMED;

    g_Camera.target_distance = WALL_L * 2;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE *const torso = item->data;
    int16_t head = 0;
    ITEM *const lara_item = Lara_GetItem();

    if (item->hit_points <= 0) {
        if (item->current_anim_state != TORSO_STATE_DEATH) {
            item->current_anim_state = TORSO_STATE_DEATH;
            Item_SwitchToAnim(item, TORSO_DIE_ANIM, 0);
        }
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        Creature_Mood(item, &info, true);

        int16_t angle =
            Math_Atan(
                torso->target.z - item->pos.z, torso->target.x - item->pos.x)
            - item->rot.y;

        if (item->touch_bits) {
            Lara_TakeDamage(TORSO_TOUCH_DAMAGE, true);
        }

        switch (item->current_anim_state) {
        case TORSO_STATE_SET:
            item->goal_anim_state = TORSO_STATE_FALL;
            item->gravity = true;
            break;

        case TORSO_STATE_STOP:
            if (lara_item->hit_points <= 0) {
                break;
            }

            torso->flags = 0;
            if (angle > TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STATE_TURN_R;
            } else if (angle < -TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STATE_TURN_L;
            } else if (info.distance >= TORSO_ATTACK_RANGE) {
                item->goal_anim_state = TORSO_STATE_FORWARD;
            } else if (lara_item->hit_points > TORSO_ATTACK_DAMAGE) {
                if (Random_GetControl() < 0x4000) {
                    item->goal_anim_state = TORSO_STATE_ATTACK_1;
                } else {
                    item->goal_anim_state = TORSO_STATE_ATTACK_2;
                }
            } else if (info.distance < TORSO_CLOSE_RANGE) {
                item->goal_anim_state = TORSO_STATE_ATTACK_3;
            } else {
                item->goal_anim_state = TORSO_STATE_FORWARD;
            }
            break;

        case TORSO_STATE_FORWARD:
            if (angle < -TORSO_TURN) {
                item->goal_anim_state -= TORSO_TURN;
            } else if (angle > TORSO_TURN) {
                item->goal_anim_state += TORSO_TURN;
            } else {
                item->goal_anim_state += angle;
            }

            if (angle > TORSO_NEED_TURN || angle < -TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STATE_STOP;
            } else if (info.distance < TORSO_ATTACK_RANGE) {
                item->goal_anim_state = TORSO_STATE_STOP;
            }
            break;

        case TORSO_STATE_TURN_L:
            if (!torso->flags) {
                torso->flags = item->frame_num;
            } else if (
                Item_TestAnimEqual(item, TORSO_TURN_L_ANIM)
                && Item_TestFrameRange(
                    item, TORSO_FRAME_TURN_L_START, TORSO_FRAME_TURN_L_END)) {
                item->rot.y -= DEG_1 * 9;
            }

            if (angle > -TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STATE_STOP;
            }
            break;

        case TORSO_STATE_TURN_R:
            if (!torso->flags) {
                torso->flags = item->frame_num;
            } else if (
                Item_TestAnimEqual(item, TORSO_TURN_R_ANIM)
                && Item_TestFrameRange(
                    item, TORSO_FRAME_TURN_R_START, TORSO_FRAME_TURN_R_END)) {
                item->rot.y += DEG_1 * 14;
            }

            if (angle < TORSO_NEED_TURN) {
                item->goal_anim_state = TORSO_STATE_STOP;
            }
            break;

        case TORSO_STATE_ATTACK_1:
            if (!torso->flags && (item->touch_bits & TORSO_TRIGHT)) {
                Lara_TakeDamage(TORSO_ATTACK_DAMAGE, true);
                torso->flags = 1;
            }
            break;

        case TORSO_STATE_ATTACK_2:
            if (!torso->flags && (item->touch_bits & TORSO_TOUCH)) {
                Lara_TakeDamage(TORSO_ATTACK_DAMAGE, true);
                torso->flags = 1;
            }
            break;

        case TORSO_STATE_ATTACK_3:
            if ((item->touch_bits & TORSO_TRIGHT)
                || lara_item->hit_points <= 0) {
                item->goal_anim_state = TORSO_STATE_KILL;
                M_KillLara(item);
            }
            break;

        case TORSO_STATE_KILL:
            g_Camera.target_distance = WALL_L * 2;
            g_Camera.flags = CF_FOLLOW_CENTRE;
            break;
        }
    }

    Creature_Head(item, head);

    if (item->current_anim_state == TORSO_STATE_FALL) {
        Item_Animate(item);

        if (item->pos.y > item->floor) {
            item->goal_anim_state = TORSO_STATE_STOP;
            item->gravity = false;
            item->pos.y = item->floor;
            g_Camera.bounce = 500;
        }
    } else {
        Creature_Animate(item_num, 0, 0);
    }

    if (item->status == IS_DEACTIVATED) {
        Sound_Effect(SFX_ATLANTEAN_DEATH, &item->pos, SPM_NORMAL);
        Item_Explode(item_num, -1, TORSO_PART_DAMAGE);
        Room_TestTriggers(item);

        Item_Kill(item_num);
        item->status = IS_DEACTIVATED;
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise_func = Creature_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = TORSO_HITPOINTS;
    obj->radius = TORSO_RADIUS;
    obj->smartness = TORSO_SMARTNESS;
    obj->intelligent = true;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 1)->rot.y = true;
}

REGISTER_OBJECT(O_TORSO, M_Setup)
