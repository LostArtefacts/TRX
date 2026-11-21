#include <trx/game/collision.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

#define M_HELICOPTER_OCB 666

typedef struct {
    bool collidable;
} M_PRIV;

static int32_t M_GetOCB(const ITEM *item);
static void M_ControlHelicopter(ITEM *item);

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value = {};
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)
        || value.type != TVT_S32) {
        return 0;
    }
    return value.as_int;
}

static void M_ControlHelicopter(ITEM *const item)
{
    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);
    Sound_Effect(SFX_HELICOPTER_LOOP, &pos, SPM_NORMAL);
    if (item->frame_num == Item_GetAnim(item)->frame_end) {
        item->trigger.mask = 0;
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
    if (M_GetOCB(item) == M_HELICOPTER_OCB) {
        M_ControlHelicopter(item);
    }

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, collidable, true,
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
