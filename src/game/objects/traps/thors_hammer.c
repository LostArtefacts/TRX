#include "game/objects/traps/thors_hammer.h"

#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/traps/movable_block.h"
#include "game/room.h"
#include "global/vars.h"

typedef enum {
    THS_SET = 0,
    THS_TEASE = 1,
    THS_ACTIVE = 2,
    THS_DONE = 3,
} THOR_HAMMER_STATE;

void ThorsHandle_Setup(OBJECT_INFO *obj)
{
    obj->initialise = ThorsHandle_Initialise;
    obj->control = ThorsHandle_Control;
    obj->draw_routine = DrawUnclippedItem;
    obj->collision = ThorsHandle_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void ThorsHead_Setup(OBJECT_INFO *obj)
{
    obj->collision = ThorsHead_Collision;
    obj->draw_routine = DrawUnclippedItem;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void ThorsHandle_Initialise(int16_t item_num)
{
    ITEM_INFO *hand_item = &g_Items[item_num];
    int16_t head_item_num = Item_Create();
    ITEM_INFO *head_item = &g_Items[head_item_num];
    head_item->object_number = O_THORS_HEAD;
    head_item->room_number = hand_item->room_number;
    head_item->pos = hand_item->pos;
    head_item->shade = hand_item->shade;
    Item_Initialise(head_item_num);
    hand_item->data = head_item;
    g_LevelItemCount++;
}

void ThorsHandle_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    switch (item->current_anim_state) {
    case THS_SET:
        if (TriggerActive(item)) {
            item->goal_anim_state = THS_TEASE;
        } else {
            RemoveActiveItem(item_num);
            item->status = IS_NOT_ACTIVE;
        }
        break;

    case THS_TEASE:
        if (TriggerActive(item)) {
            item->goal_anim_state = THS_ACTIVE;
        } else {
            item->goal_anim_state = THS_SET;
        }
        break;

    case THS_ACTIVE: {
        int32_t frm =
            item->frame_number - g_Anims[item->anim_number].frame_base;
        if (frm > 30) {
            int32_t x = item->pos.x;
            int32_t z = item->pos.z;

            switch (item->pos.y_rot) {
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
                g_LaraItem->gravity_status = 0;
                g_LaraItem->current_anim_state = LS_SPECIAL;
                g_LaraItem->goal_anim_state = LS_SPECIAL;
                g_LaraItem->anim_number = LA_ROLLING_BALL_DEATH;
                g_LaraItem->frame_number = AF_RBALL_DEATH;
            }
        }
        break;
    }

    case THS_DONE: {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;
        int32_t old_x = x;
        int32_t old_z = z;

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor = Room_GetFloor(x, item->pos.y, z, &room_num);
        Room_GetHeight(floor, x, item->pos.y, z);
        TestTriggers(g_TriggerIndex, 1);

        switch (item->pos.y_rot) {
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
            AlterFloorHeight(item, -WALL_L * 2);
        }
        item->pos.x = old_x;
        item->pos.z = old_z;

        RemoveActiveItem(item_num);
        item->status = IS_DEACTIVATED;
        break;
    }
    }
    AnimateItem(item);

    ITEM_INFO *head_item = item->data;
    int32_t anim = item->anim_number - g_Objects[O_THORS_HANDLE].anim_index;
    int32_t frm = item->frame_number - g_Anims[item->anim_number].frame_base;
    head_item->anim_number = g_Objects[O_THORS_HEAD].anim_index + anim;
    head_item->frame_number = g_Anims[head_item->anim_number].frame_base + frm;
    head_item->current_anim_state = item->current_anim_state;
}

void ThorsHandle_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
}

void ThorsHead_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push && item->current_anim_state != THS_ACTIVE) {
        Lara_Push(item, coll, false, true);
    }
}
