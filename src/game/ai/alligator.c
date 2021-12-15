#include "game/ai/crocodile.h"

#include "game/ai/alligator.h"
#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lot.h"
#include "global/vars.h"

void SetupAlligator(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = AlligatorControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 3;
    obj->hit_points = ALLIGATOR_HITPOINTS;
    obj->pivot_length = 600;
    obj->radius = ALLIGATOR_RADIUS;
    obj->smartness = ALLIGATOR_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 28] |= BEB_ROT_Y;
}

void AlligatorControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *gator = item->data;
    FLOOR_INFO *floor;
    int16_t head = 0;
    int16_t angle = 0;
    int16_t room_num;
    int32_t wh;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != ALLIGATOR_DEATH) {
            item->current_anim_state = ALLIGATOR_DEATH;
            item->anim_number =
                g_Objects[O_ALLIGATOR].anim_index + ALLIGATOR_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            item->hit_points = DONT_TARGET;
        }

        wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_CROCODILE;
            item->current_anim_state = CROCODILE_DEATH;
            item->goal_anim_state = CROCODILE_DEATH;
            item->anim_number =
                g_Objects[O_CROCODILE].anim_index + CROCODILE_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            room_num = item->room_number;
            floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
            item->pos.y =
                GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            item->pos.x_rot = 0;
        } else if (item->pos.y > wh + ALLIGATOR_FLOAT_SPEED) {
            item->pos.y -= ALLIGATOR_FLOAT_SPEED;
        } else if (item->pos.y < wh) {
            item->pos.y = wh;
            if (gator) {
                DisableBaddieAI(item_num);
            }
        }

        AnimateItem(item);

        room_num = item->room_number;
        floor = GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
        if (room_num != item->room_number) {
            ItemNewRoom(item_num, room_num);
        }
        return;
    }

    AI_INFO info;
    CreatureAIInfo(item, &info);

    if (info.ahead) {
        head = info.angle;
    }

    CreatureMood(item, &info, 1);
    CreatureTurn(item, ALLIGATOR_TURN);

    switch (item->current_anim_state) {
    case ALLIGATOR_SWIM:
        if (info.bite && item->touch_bits) {
            item->goal_anim_state = ALLIGATOR_ATTACK;
            if (g_Config.fix_alligator_ai) {
                item->required_anim_state = ALLIGATOR_SWIM;
            }
        }
        break;

    case ALLIGATOR_ATTACK:
        if (item->frame_number
            == (g_Config.fix_alligator_ai
                    ? ALLIGATOR_BITE_AF
                    : g_Anims[item->anim_number].frame_base)) {
            item->required_anim_state = ALLIGATOR_EMPTY;
        }

        if (info.bite && item->touch_bits) {
            if (item->required_anim_state == ALLIGATOR_EMPTY) {
                CreatureEffect(item, &g_CrocodileBite, DoBloodSplat);
                g_LaraItem->hit_points -= ALLIGATOR_BITE_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->required_anim_state = ALLIGATOR_SWIM;
            }
            if (g_Config.fix_alligator_ai) {
                item->goal_anim_state = ALLIGATOR_SWIM;
            }
        } else {
            item->goal_anim_state = ALLIGATOR_SWIM;
        }
        break;
    }

    CreatureHead(item, head);

    wh = GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh == NO_HEIGHT) {
        item->object_number = O_CROCODILE;
        item->current_anim_state =
            g_Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->anim_number = g_Objects[O_CROCODILE].anim_index;
        item->frame_number = g_Anims[item->anim_number].frame_base;
        item->pos.y = item->floor;
        item->pos.x_rot = 0;
        gator->LOT.step = STEP_L;
        gator->LOT.drop = -STEP_L;
        gator->LOT.fly = 0;
    } else if (item->pos.y < wh + STEP_L) {
        item->pos.y = wh + STEP_L;
    }

    CreatureAnimation(item_num, angle, 0);
}
