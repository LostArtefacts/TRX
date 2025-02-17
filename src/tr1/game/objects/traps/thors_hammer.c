#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/debug.h>

typedef enum {
    THOR_HAMMER_STATE_SET = 0,
    THOR_HAMMER_STATE_TEASE = 1,
    THOR_HAMMER_STATE_ACTIVE = 2,
    THOR_HAMMER_STATE_DONE = 3,
} THOR_HAMMER_STATE;

static void M_SetupHandle(OBJECT *obj);
static void M_InitialiseHandle(int16_t item_num);
static void M_ControlHandle(int16_t item_num);
static void M_CollisionHandle(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static void M_SetupHead(OBJECT *obj);
static void M_CollisionHead(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

static void M_SetupHandle(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseHandle;
    obj->control_func = M_ControlHandle;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = M_CollisionHandle;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_InitialiseHandle(const int16_t item_num)
{
    ITEM *const hand_item = Item_Get(item_num);
    const int16_t head_item_num = Item_CreateLevelItem();
    ASSERT(head_item_num != NO_ITEM);
    ITEM *const head_item = Item_Get(head_item_num);
    head_item->object_id = O_THORS_HEAD;
    head_item->room_num = hand_item->room_num;
    head_item->pos = hand_item->pos;
    head_item->rot = hand_item->rot;
    head_item->shade.value_1 = hand_item->shade.value_1;
    Item_Initialise(head_item_num);
    hand_item->data = head_item;
}

static void M_ControlHandle(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->current_anim_state) {
    case THOR_HAMMER_STATE_SET:
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = THOR_HAMMER_STATE_TEASE;
        } else {
            Item_RemoveActive(item_num);
            item->status = IS_INACTIVE;
        }
        break;

    case THOR_HAMMER_STATE_TEASE:
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = THOR_HAMMER_STATE_ACTIVE;
        } else {
            item->goal_anim_state = THOR_HAMMER_STATE_SET;
        }
        break;

    case THOR_HAMMER_STATE_ACTIVE: {
        const int32_t frame_num = Item_GetRelativeFrame(item);
        if (frame_num > 30) {
            int32_t x = item->pos.x;
            int32_t z = item->pos.z;

            switch (item->rot.y) {
            case 0:
                z += WALL_L * 3;
                break;
            case DEG_90:
                x += WALL_L * 3;
                break;
            case -DEG_90:
                x -= WALL_L * 3;
                break;
            case -DEG_180:
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

    case THOR_HAMMER_STATE_DONE: {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;
        int32_t old_x = x;
        int32_t old_z = z;

        Room_TestTriggers(item);

        switch (item->rot.y) {
        case 0:
            z += WALL_L * 3;
            break;
        case DEG_90:
            x += WALL_L * 3;
            break;
        case -DEG_90:
            x -= WALL_L * 3;
            break;
        case -DEG_180:
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

    ITEM *const head_item = item->data;
    const int16_t relative_anim = Item_GetRelativeAnim(item);
    const int16_t relative_frame = Item_GetRelativeFrame(item);
    Item_SwitchToAnim(head_item, relative_anim, relative_frame);
    head_item->current_anim_state = item->current_anim_state;
}

static void M_CollisionHandle(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
}

static void M_SetupHead(OBJECT *const obj)
{
    obj->collision_func = M_CollisionHead;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_CollisionHead(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push
        && item->current_anim_state != THOR_HAMMER_STATE_ACTIVE) {
        Lara_Push(item, coll, false, true);
    }
}

REGISTER_OBJECT(O_THORS_HANDLE, M_SetupHandle)
REGISTER_OBJECT(O_THORS_HEAD, M_SetupHead)
