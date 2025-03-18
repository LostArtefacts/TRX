#include "game/items.h"
#include "game/objects/common.h"
#include "game/sound.h"
#include "global/vars.h"

typedef enum {
    // clang-format off
    DART_EMITTER_STATE_IDLE   = 0,
    DART_EMITTER_STATE_FIRE   = 1,
    DART_EMITTER_STATE_RELOAD = 2,
    // clang-format on
} DART_EMITTER_STATE;

static void M_CreateDart(ITEM *item);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_CreateDart(ITEM *const item)
{
    const int16_t dart_item_num = Item_Create();
    if (dart_item_num == NO_ITEM) {
        return;
    }

    ITEM *const dart_item = Item_Get(dart_item_num);
    dart_item->object_id = O_DART;
    dart_item->room_num = item->room_num;
    dart_item->shade.value_1 = -1;
    dart_item->rot.y = item->rot.y;
    dart_item->pos.y = item->pos.y - 512;

    int32_t x = 0;
    int32_t z = 0;
    switch (dart_item->rot.y) {
    case 0:
        z = -412;
        break;
    case 16364:
        x = -412;
        break;
    case -16384:
        x = 412;
        break;
    case -32768:
        z = 412;
        break;
    }

    dart_item->pos.x = item->pos.x + x;
    dart_item->pos.z = item->pos.z + z;
    Item_Initialise(dart_item_num);
    Item_AddActive(dart_item_num);

    dart_item->status = IS_ACTIVE;
    Sound_Effect(SFX_CIRCLE_BLADE, &dart_item->pos, SPM_NORMAL);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DART_EMITTER_STATE_IDLE) {
            item->goal_anim_state = DART_EMITTER_STATE_FIRE;
        }
    } else if (item->current_anim_state == DART_EMITTER_STATE_FIRE) {
        item->goal_anim_state = DART_EMITTER_STATE_IDLE;
    }

    if (item->current_anim_state == DART_EMITTER_STATE_FIRE
        && Item_TestFrameEqual(item, 0)) {
        M_CreateDart(item);
    }

    Item_Animate(item);
}

REGISTER_OBJECT(O_DART_EMITTER, M_Setup)
