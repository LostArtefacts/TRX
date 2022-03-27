#include "game/objects/ai/pierre.h"

#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/items.h"
#include "game/lot.h"
#include "game/people.h"
#include "game/random.h"
#include "global/vars.h"

BITE_INFO g_PierreGun1 = { 60, 200, 0, 11 };
BITE_INFO g_PierreGun2 = { -57, 200, 0, 14 };
int16_t g_PierreItemNum = NO_ITEM;

void Pierre_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = Pierre_Control;
    obj->collision = CreatureCollision;
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

    if (g_PierreItemNum == NO_ITEM) {
        g_PierreItemNum = item_num;
    } else if (g_PierreItemNum != item_num) {
        if (item->flags & IF_ONESHOT) {
            KillItem(g_PierreItemNum);
        } else {
            KillItem(item_num);
        }
    }

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
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
            item->anim_number =
                g_Objects[O_PIERRE].anim_index + PIERRE_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            SpawnItem(item, O_MAGNUM_ITEM);
            SpawnItem(item, O_SCION_ITEM2);
            SpawnItem(item, O_KEY_ITEM1);
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        if (pierre->flags) {
            info.enemy_zone = -1;
            item->hit_status = 1;
        }
        CreatureMood(item, &info, 0);

        angle = CreatureTurn(item, pierre->maximum_turn);

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
            } else if (Targetable(item, &info)) {
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
            } else if (Targetable(item, &info)) {
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
            } else if (Targetable(item, &info)) {
                item->goal_anim_state = PIERRE_SHOOT;
            } else {
                item->goal_anim_state = PIERRE_STOP;
            }
            break;

        case PIERRE_SHOOT:
            if (!item->required_anim_state) {
                if (ShotLara(item, info.distance, &g_PierreGun1, head)) {
                    g_LaraItem->hit_points -= PIERRE_SHOT_DAMAGE / 2;
                    g_LaraItem->hit_status = 1;
                }
                if (ShotLara(item, info.distance, &g_PierreGun2, head)) {
                    g_LaraItem->hit_points -= PIERRE_SHOT_DAMAGE / 2;
                    g_LaraItem->hit_status = 1;
                }
                item->required_anim_state = PIERRE_AIM;
            }
            if (pierre->mood == MOOD_ESCAPE
                && Random_GetControl() > PIERRE_WIMP_CHANCE) {
                item->required_anim_state = PIERRE_STOP;
            }
            break;
        }
    }

    CreatureTilt(item, tilt);
    CreatureHead(item, head);
    CreatureAnimation(item_num, angle, 0);

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

        if (LOS(&start, &target)) {
            pierre->flags = 1;
        } else if (pierre->flags > PIERRE_DISAPPEAR) {
            item->hit_points = DONT_TARGET;
            DisableBaddieAI(item_num);
            KillItem(item_num);
            g_PierreItemNum = NO_ITEM;
        }
    }

    int16_t wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT) {
        item->hit_points = DONT_TARGET;
        DisableBaddieAI(item_num);
        KillItem(item_num);
        g_PierreItemNum = NO_ITEM;
    }
}
