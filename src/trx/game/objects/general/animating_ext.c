#include <trx/game/objects.h>
#include <trx/game/rooms.h>

typedef enum {
    // clang-format off
    M_STATE_ON  = 0,
    M_STATE_OFF = 1,
    // clang-format on
} M_STATE;

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status == IS_ACTIVE && item->current_anim_state == M_STATE_OFF) {
        Object_Collision(item_num, lara_item, coll);
    } else {
        Object_Collision_Trap(item_num, lara_item, coll);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item) && !(item->flags & IF_ONE_SHOT)) {
        item->goal_anim_state = M_STATE_ON;
    } else if (item->goal_anim_state != M_STATE_OFF) {
        item->goal_anim_state = M_STATE_OFF;
    }

    Item_Animate(item);

    if (Object_Get(item->object_id)->mesh_count > 0) {
        XYZ_32 pos = {};
        Collide_GetJointAbsPosition(item, &pos, 0);
        int16_t room_num = item->room_num;
        Room_GetSector(pos, &room_num);
        Item_UpdateRoom(item_num, room_num);
    }

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        item->collidable = false;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_ANIMATING_EXT_1, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_2, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_3, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_4, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_5, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_6, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_7, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_8, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_9, M_Setup)
REGISTER_OBJECT(O_ANIMATING_EXT_10, M_Setup)
