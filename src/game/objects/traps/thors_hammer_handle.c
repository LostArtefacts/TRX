#include "game/objects/traps/thors_hammer_handle.h"

#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/vars.h"

typedef enum {
    THS_SET = 0,
    THS_TEASE = 1,
    THS_ACTIVE = 2,
    THS_DONE = 3,
} THOR_HAMMER_STATE;

void ThorsHammerHandle_Setup(OBJECT *obj)
{
    obj->initialise = ThorsHammerHandle_Initialise;
    obj->control = ThorsHammerHandle_Control;
    obj->draw_routine = Object_DrawUnclippedItem;
    obj->collision = ThorsHammerHandle_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void ThorsHammerHandle_Initialise(int16_t item_num)
{
    ITEM *hand_item = &g_Items[item_num];
    int16_t head_item_num = Item_Create();
    ITEM *head_item = &g_Items[head_item_num];
    head_item->object_id = O_THORS_HEAD;
    head_item->room_num = hand_item->room_num;
    head_item->pos = hand_item->pos;
    head_item->rot = hand_item->rot;
    head_item->shade = hand_item->shade;
    Item_Initialise(head_item_num);
    hand_item->data = head_item;
    g_LevelItemCount++;
}

void ThorsHammerHandle_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    switch (item->current_anim_state) {
    case THS_SET:
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = THS_TEASE;
        } else {
            Item_RemoveActive(item_num);
            item->status = IS_INACTIVE;
        }
        break;

    case THS_TEASE:
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = THS_ACTIVE;
        } else {
            item->goal_anim_state = THS_SET;
        }
        break;

    case THS_ACTIVE: {
        int32_t frm = item->frame_num - g_Anims[item->anim_num].frame_base;
        if (frm > 30) {
            int32_t x = item->pos.x;
            int32_t z = item->pos.z;

            switch (item->rot.y) {
            case 0:
                z += WALL_L * 3;
                break;
            case PHD_90:
                x += WALL_L * 3;
                break;
            case -PHD_90:
                x -= WALL_L * 3;
                break;
            case -PHD_180:
                z -= WALL_L * 3;
                break;
            }

            if (g_LaraItem->hit_points >= 0 && g_LaraItem->pos.x > x - 520
                && g_LaraItem->pos.x < x + 520 && g_LaraItem->pos.z > z - 520
                && g_LaraItem->pos.z < z + 520) {
                g_LaraItem->hit_points = -1;
                g_LaraItem->pos.y = item->pos.y;
                g_LaraItem->gravity = 0;
                g_LaraItem->current_anim_state = LS_SPECIAL;
                g_LaraItem->goal_anim_state = LS_SPECIAL;
                Item_SwitchToAnim(g_LaraItem, LA_ROLLING_BALL_DEATH, 0);
            }
        }
        break;
    }

    case THS_DONE: {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;
        int32_t old_x = x;
        int32_t old_z = z;

        Room_TestTriggers(item);

        switch (item->rot.y) {
        case 0:
            z += WALL_L * 3;
            break;
        case PHD_90:
            x += WALL_L * 3;
            break;
        case -PHD_90:
            x -= WALL_L * 3;
            break;
        case -PHD_180:
            z -= WALL_L * 3;
            break;
        }

        item->pos.x = x;
        item->pos.z = z;
        if (g_LaraItem->hit_points >= 0) {
            Room_AlterFloorHeight(item, -WALL_L * 2);
        }
        item->pos.x = old_x;
        item->pos.z = old_z;

        Item_RemoveActive(item_num);
        item->status = IS_DEACTIVATED;
        break;
    }
    }
    Item_Animate(item);

    ITEM *head_item = item->data;
    int16_t relative_anim =
        item->anim_num - g_Objects[item->object_id].anim_idx;
    int16_t relative_frame =
        item->frame_num - g_Anims[item->anim_num].frame_base;
    Item_SwitchToAnim(head_item, relative_anim, relative_frame);
    head_item->current_anim_state = item->current_anim_state;
}

void ThorsHammerHandle_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *item = &g_Items[item_num];
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
}
