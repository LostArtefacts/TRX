#include "game/traps/thors_hammer.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/traps/movable_block.h"
#include "global/vars.h"

void SetupThorsHandle(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseThorsHandle;
    obj->control = ThorsHandleControl;
    obj->draw_routine = DrawUnclippedItem;
    obj->collision = ThorsHandleCollision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void SetupThorsHead(OBJECT_INFO *obj)
{
    obj->collision = ThorsHeadCollision;
    obj->draw_routine = DrawUnclippedItem;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void InitialiseThorsHandle(int16_t item_num)
{
    ITEM_INFO *hand_item = &Items[item_num];
    int16_t head_item_num = CreateItem();
    ITEM_INFO *head_item = &Items[head_item_num];
    head_item->object_number = O_THORS_HEAD;
    head_item->room_number = hand_item->room_number;
    head_item->pos = hand_item->pos;
    head_item->shade = hand_item->shade;
    InitialiseItem(head_item_num);
    hand_item->data = head_item;
    LevelItemCount++;
}

void ThorsHandleControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

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
        int32_t frm = item->frame_number - Anims[item->anim_number].frame_base;
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

            if (LaraItem->hit_points >= 0 && LaraItem->pos.x > x - 520
                && LaraItem->pos.x < x + 520 && LaraItem->pos.z > z - 520
                && LaraItem->pos.z < z + 520) {
                LaraItem->hit_points = -1;
                LaraItem->pos.y = item->pos.y;
                LaraItem->gravity_status = 0;
                LaraItem->current_anim_state = AS_SPECIAL;
                LaraItem->goal_anim_state = AS_SPECIAL;
                LaraItem->anim_number = AA_RBALL_DEATH;
                LaraItem->frame_number = AF_RBALL_DEATH;
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
        FLOOR_INFO *floor = GetFloor(x, item->pos.y, z, &room_num);
        GetHeight(floor, x, item->pos.y, z);
        TestTriggers(TriggerIndex, 1);

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
        if (LaraItem->hit_points >= 0) {
            AlterFloorHeight(item, -2 * WALL_L);
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
    int32_t anim = item->anim_number - Objects[O_THORS_HANDLE].anim_index;
    int32_t frm = item->frame_number - Anims[item->anim_number].frame_base;
    head_item->anim_number = Objects[O_THORS_HEAD].anim_index + anim;
    head_item->frame_number = Anims[head_item->anim_number].frame_base + frm;
    head_item->current_anim_state = item->current_anim_state;
}

void ThorsHandleCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        ItemPushLara(item, lara_item, coll, 0, 1);
    }
}

void ThorsHeadCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];
    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push && item->current_anim_state != THS_ACTIVE) {
        ItemPushLara(item, lara_item, coll, 0, 1);
    }
}
