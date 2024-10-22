#include "game/objects/general/lift.h"

#include "game/items.h"
#include "game/room.h"
#include "global/funcs.h"

#define LIFT_WAIT_TIME (3 * FRAMES_PER_SECOND) // = 90
#define LIFT_SHIFT 16
#define LIFT_TRAVEL_DIST (STEP_L * 22)

typedef enum {
    LIFT_STATE_DOOR_CLOSED = 0,
    LIFT_STATE_DOOR_OPEN = 1,
} LIFT_STATE;

void __cdecl Lift_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    LIFT_INFO *const lift_data =
        game_malloc(sizeof(LIFT_INFO), GBUF_TEMP_ALLOC);
    lift_data->start_height = item->pos.y;
    lift_data->wait_time = 0;

    item->data = lift_data;
}

void __cdecl Lift_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    LIFT_INFO *const lift_data = item->data;

    if (Item_IsTriggerActive(item)) {
        if (item->pos.y
            < lift_data->start_height + LIFT_TRAVEL_DIST - LIFT_SHIFT) {
            if (lift_data->wait_time < LIFT_WAIT_TIME) {
                item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
                lift_data->wait_time++;
            } else {
                item->goal_anim_state = LIFT_STATE_DOOR_CLOSED;
                item->pos.y += LIFT_SHIFT;
            }
        } else {
            item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
            lift_data->wait_time = 0;
        }
    } else {
        if (item->pos.y > lift_data->start_height + LIFT_SHIFT) {
            if (lift_data->wait_time < LIFT_WAIT_TIME) {
                item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
                lift_data->wait_time++;
            } else {
                item->goal_anim_state = LIFT_STATE_DOOR_CLOSED;
                item->pos.y -= LIFT_SHIFT;
            }
        } else {
            item->goal_anim_state = LIFT_STATE_DOOR_OPEN;
            lift_data->wait_time = 0;
        }
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }
}
