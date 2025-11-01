#include <trx/game/camera.h>
#include <trx/game/objects.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/version.h>

static void M_ActivateRelatedItem(const int16_t item_num)
{
    ITEM *const earth_item = Item_Get(item_num);
    Item_AddActive(item_num);
    earth_item->status = IS_ACTIVE;
    earth_item->flags = IF_CODE_BITS;
    earth_item->timer = 0;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (g_TRVersion == 1) {
        if (Item_IsTriggerActive(item)) {
            if (Random_GetDraw() < 256) {
                g_Camera.bounce = -150;
                Sound_Effect(SFX_EARTHQUAKE_1, nullptr, SPM_NORMAL);
            } else if (Random_GetControl() < 1024) {
                g_Camera.bounce = 50;
                Sound_Effect(SFX_EARTHQUAKE_2, nullptr, SPM_NORMAL);
            }
        }
    } else {
        if (Random_GetDraw() < 512) {
            Sound_Effect(SFX_EARTHQUAKE_1, nullptr, SPM_NORMAL);
            g_Camera.bounce = -200;
        }
    }

    OBJECT_ID object_id_to_activate = NO_OBJECT;
    const int32_t random = Random_GetControl();
    if (random < 512) {
        object_id_to_activate = O_FLAME_EMITTER;
    } else if (random < 1024) {
        object_id_to_activate = O_FALLING_CEILING_1;
    }
    if (object_id_to_activate == NO_OBJECT
        || !Object_Get(object_id_to_activate)->loaded) {
        return;
    }

    int16_t related_item_num = Room_Get(item->room_num)->item_num;
    while (related_item_num != NO_ITEM) {
        const ITEM *const earth_item = Item_Get(related_item_num);
        if (earth_item->object_id == object_id_to_activate
            && earth_item->status != IS_ACTIVE
            && earth_item->status != IS_DEACTIVATED) {
            M_ActivateRelatedItem(related_item_num);
            break;
        }
        related_item_num = earth_item->next_item;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_EARTHQUAKE, M_Setup)
