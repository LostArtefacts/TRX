#include "game/effects.h"
#include "game/items.h"
#include "game/sound.h"

typedef enum {
    DART_EMITTER_STATE_IDLE = 0,
    DART_EMITTER_STATE_FIRE = 1,
} DART_EMITTER_STATE;

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

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
    } else {
        if (item->current_anim_state == DART_EMITTER_STATE_FIRE) {
            item->goal_anim_state = DART_EMITTER_STATE_IDLE;
        }
    }

    if (item->current_anim_state == DART_EMITTER_STATE_FIRE
        && Item_TestFrameEqual(item, 0)) {
        int16_t dart_item_num = Item_Create();
        if (dart_item_num != NO_ITEM) {
            ITEM *const dart = Item_Get(dart_item_num);
            dart->object_id = O_DART;
            dart->room_num = item->room_num;
            dart->shade.value_1 = -1;
            dart->rot.y = item->rot.y;
            dart->pos.y = item->pos.y - WALL_L / 2;

            int32_t x = 0;
            int32_t z = 0;
            switch (dart->rot.y) {
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

            dart->pos.x = item->pos.x + x;
            dart->pos.z = item->pos.z + z;
            Item_Initialise(dart_item_num);
            Item_AddActive(dart_item_num);
            dart->status = IS_ACTIVE;

            int16_t effect_num = Effect_Create(dart->room_num);
            if (effect_num != NO_EFFECT) {
                EFFECT *effect = Effect_Get(effect_num);
                effect->pos = dart->pos;
                effect->rot = dart->rot;
                effect->speed = 0;
                effect->frame_num = 0;
                effect->counter = 0;
                effect->object_id = O_DART_EFFECT;
                Sound_Effect(SFX_DARTS, &effect->pos, SPM_NORMAL);
            }
        }
    }
    Item_Animate(item);
}

REGISTER_OBJECT(O_DART_EMITTER, M_Setup)
