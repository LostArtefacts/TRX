#include "game/ai/vole.h"

#include "game/ai/rat.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/lot.h"
#include "global/vars.h"

void SetupVole(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseCreature;
    obj->control = VoleControl;
    obj->collision = CreatureCollision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = RAT_HITPOINTS;
    obj->pivot_length = 200;
    obj->radius = RAT_RADIUS;
    obj->smartness = RAT_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
    g_AnimBones[obj->bone_index + 4] |= BEB_ROT_Y;
}

void VoleControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    int16_t head = 0;
    int16_t angle = 0;

    if (item->hit_points <= 0) {
        if (item->current_anim_state != VOLE_DEATH) {
            item->current_anim_state = VOLE_DEATH;
            item->anim_number = g_Objects[O_VOLE].anim_index + VOLE_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
        }

        CreatureHead(item, head);

        AnimateItem(item);

        int32_t wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_RAT;
            item->current_anim_state = RAT_DEATH;
            item->goal_anim_state = RAT_DEATH;
            item->anim_number = g_Objects[O_RAT].anim_index + RAT_DIE_ANIM;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            item->pos.y = item->floor;
        }
    } else {
        AI_INFO info;
        CreatureAIInfo(item, &info);

        if (info.ahead) {
            head = info.angle;
        }

        CreatureMood(item, &info, 1);

        angle = CreatureTurn(item, VOLE_SWIM_TURN);

        switch (item->current_anim_state) {
        case VOLE_SWIM:
            if (info.ahead && (item->touch_bits & RAT_TOUCH)) {
                item->goal_anim_state = VOLE_ATTACK;
            }
            break;

        case VOLE_ATTACK:
            if (item->required_anim_state == VOLE_EMPTY && info.ahead
                && (item->touch_bits & RAT_TOUCH)) {
                CreatureEffect(item, &g_RatBite, Blood_Spawn);
                g_LaraItem->hit_points -= RAT_BITE_DAMAGE;
                g_LaraItem->hit_status = 1;
                item->required_anim_state = VOLE_SWIM;
            }
            item->goal_anim_state = VOLE_EMPTY;
            break;
        }

        CreatureHead(item, head);

        int32_t wh = GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_number);
        if (wh == NO_HEIGHT) {
            item->object_number = O_RAT;
            item->anim_number = g_Objects[O_RAT].anim_index;
            item->frame_number = g_Anims[item->anim_number].frame_base;
            item->current_anim_state =
                g_Anims[item->anim_number].current_anim_state;
            item->goal_anim_state = item->current_anim_state;
        }

        int32_t height = item->pos.y;
        item->pos.y = item->floor;

        CreatureAnimation(item_num, angle, 0);

        if (height != NO_HEIGHT) {
            if (wh - height < -STEP_L / 8) {
                item->pos.y = height - STEP_L / 8;
            } else if (wh - height > STEP_L / 8) {
                item->pos.y = height + STEP_L / 8;
            } else {
                item->pos.y = wh;
            }
        }
    }
}
