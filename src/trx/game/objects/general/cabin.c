#include <trx/game/objects.h>
#include <trx/game/rooms.h>

#define M_DEFAULT_FLIP_SLOT 3

typedef enum {
    M_STATE_START,
    M_STATE_DROP_1,
    M_STATE_DROP_2,
    M_STATE_DROP_3,
    M_STATE_FINISH,
} M_STATE;

typedef struct {
    int32_t flip_slot;
} M_PRIV;

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->flip_slot = M_DEFAULT_FLIP_SLOT;

    OBJECT_PROPERTY_VALUE value = {};
    if (ObjectProperty_GetItemValue(item, "flip_slot", &value)
        && value.as_int < MAX_FLIP_MAPS) {
        p->flip_slot = value.as_int;
    }

    CLAMPL(p->flip_slot, -1);
}

static bool M_ShouldFlipMap(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->flip_slot >= 0;
}

static void M_FlipMap(const M_PRIV *const p)
{
    Room_SetFlipSlotFlags(p->flip_slot, IF_CODE_BITS | IF_ONE_SHOT);
    Room_FlipMap();
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        switch (item->current_anim_state) {
        case M_STATE_START:
            item->goal_anim_state = M_STATE_DROP_1;
            break;
        case M_STATE_DROP_1:
            item->goal_anim_state = M_STATE_DROP_2;
            break;
        case M_STATE_DROP_2:
            item->goal_anim_state = M_STATE_DROP_3;
            break;
        }
        item->flags = 0;
    }

    if (item->current_anim_state == M_STATE_FINISH) {
        if (M_ShouldFlipMap(item)) {
            M_FlipMap(p);
        }
        Item_Kill(item_num);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = Object_Collision;
    obj->save_anim = true;
    obj->save_flags = true;
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "flip_slot", M_DEFAULT_FLIP_SLOT,
            "The flip map slot to alter once the cabin has landed. -1 = "
            "no flipmap is performed. Value range: minimum -1; maximum 10."));
}

REGISTER_OBJECT(O_PORTACABIN, M_Setup)
