#include "game/const.h"
#include "game/effects.h"
#include "game/objects.h"
#include "game/sound.h"
#include "version.h"

typedef enum {
    // clang-format off
    DART_EMITTER_STATE_IDLE   = 0,
    DART_EMITTER_STATE_FIRE   = 1,
    DART_EMITTER_STATE_RELOAD = 2,
    // clang-format on
} M_STATE;

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
        z = -WALL_L / 2 + 100;
        break;
    case DEG_90:
        x = -WALL_L / 2 + 100;
        break;
    case -DEG_180:
        z = WALL_L / 2 - 100;
        break;
    case -DEG_90:
        x = WALL_L / 2 - 100;
        break;
    }

    dart_item->pos.x = item->pos.x + x;
    dart_item->pos.z = item->pos.z + z;
    Item_Initialise(dart_item_num);
    Item_AddActive(dart_item_num);
    dart_item->status = IS_ACTIVE;

    if (g_TRVersion == 1) {
        const int16_t effect_num = Effect_Create(dart_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos = dart_item->pos;
            effect->rot = dart_item->rot;
            effect->speed = 0;
            effect->frame_num = 0;
            effect->counter = 0;
            effect->object_id = O_DART_EFFECT;
        }
    }
    Sound_Effect(SFX_DARTS, &dart_item->pos, SPM_NORMAL);
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

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_DART_EMITTER, M_Setup)
