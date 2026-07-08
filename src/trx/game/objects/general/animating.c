#include <trx/game/objects.h>
#include <trx/game/rooms.h>

typedef struct {
    bool collidable;
} M_PRIV;

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->collidable = true;

    OBJECT_PROPERTY_VALUE value = {};
    if (ObjectProperty_GetItemValue(item, "collidable", &value)) {
        p->collidable = value.as_bool;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    if (p->collidable) {
        Object_Collision(item_num, lara_item, coll);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_BOOL(
            "collidable", true,
            "Whether or not Lara can collide with the animating."));
}

REGISTER_OBJECT(O_ANIMATING_1, M_Setup)
REGISTER_OBJECT(O_ANIMATING_2, M_Setup)
REGISTER_OBJECT(O_ANIMATING_3, M_Setup)
REGISTER_OBJECT(O_ANIMATING_4, M_Setup)
REGISTER_OBJECT(O_ANIMATING_5, M_Setup)
REGISTER_OBJECT(O_ANIMATING_6, M_Setup)
REGISTER_OBJECT(O_ANIMATING_7, M_Setup)
REGISTER_OBJECT(O_ANIMATING_8, M_Setup)
REGISTER_OBJECT(O_ANIMATING_9, M_Setup)
REGISTER_OBJECT(O_ANIMATING_10, M_Setup)
REGISTER_OBJECT(O_ANIMATING_11, M_Setup)
REGISTER_OBJECT(O_ANIMATING_12, M_Setup)
REGISTER_OBJECT(O_ANIMATING_13, M_Setup)
REGISTER_OBJECT(O_ANIMATING_14, M_Setup)
REGISTER_OBJECT(O_ANIMATING_15, M_Setup)
REGISTER_OBJECT(O_ANIMATING_16, M_Setup)
