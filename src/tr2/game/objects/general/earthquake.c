#include "game/camera.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

static void M_Activate(int16_t earth_item_num);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Activate(const int16_t earth_item_num)
{
    ITEM *const earth_item = Item_Get(earth_item_num);
    Item_AddActive(earth_item_num);
    earth_item->status = IS_ACTIVE;
    earth_item->flags = IF_CODE_BITS;
    earth_item->timer = 0;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    if (Random_GetDraw() < 512) {
        Sound_Effect(SFX_DRAGON_FEET, nullptr, SPM_NORMAL);
        g_Camera.bounce = -200;
    }

    GAME_OBJECT_ID obj_id_to_activate;
    const int32_t random = Random_GetControl();
    if (random < 512) {
        obj_id_to_activate = O_FLAME_EMITTER;
    } else if (random < 1024) {
        obj_id_to_activate = O_FALLING_CEILING;
    } else {
        return;
    }

    int16_t earth_item_num = Room_Get(item->room_num)->item_num;
    while (earth_item_num != NO_ITEM) {
        const ITEM *const earth_item = Item_Get(earth_item_num);
        if (earth_item->object_id == obj_id_to_activate
            && earth_item->status != IS_ACTIVE
            && earth_item->status != IS_DEACTIVATED) {
            M_Activate(earth_item_num);
            break;
        }
        earth_item_num = earth_item->next_item;
    }
}

REGISTER_OBJECT(O_EARTHQUAKE, M_Setup)
