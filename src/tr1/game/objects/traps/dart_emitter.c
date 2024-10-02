#include "game/objects/traps/dart_emitter.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/sound.h"

typedef enum {
    DART_EMITTER_IDLE = 0,
    DART_EMITTER_FIRE = 1,
} DART_EMITTER_STATE;

void DartEmitter_Setup(OBJECT *obj)
{
    obj->control = DartEmitter_Control;
    obj->save_flags = 1;
}

void DartEmitter_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DART_EMITTER_IDLE) {
            item->goal_anim_state = DART_EMITTER_FIRE;
        }
    } else {
        if (item->current_anim_state == DART_EMITTER_FIRE) {
            item->goal_anim_state = DART_EMITTER_IDLE;
        }
    }

    if (item->current_anim_state == DART_EMITTER_FIRE
        && Item_TestFrameEqual(item, 0)) {
        int16_t dart_item_num = Item_Create();
        if (dart_item_num != NO_ITEM) {
            ITEM *dart = &g_Items[dart_item_num];
            dart->object_id = O_DARTS;
            dart->room_num = item->room_num;
            dart->shade = -1;
            dart->rot.y = item->rot.y;
            dart->pos.y = item->pos.y - WALL_L / 2;

            int32_t x = 0;
            int32_t z = 0;
            switch (dart->rot.y) {
            case 0:
                z = -WALL_L / 2 + 100;
                break;
            case PHD_90:
                x = -WALL_L / 2 + 100;
                break;
            case -PHD_180:
                z = WALL_L / 2 - 100;
                break;
            case -PHD_90:
                x = WALL_L / 2 - 100;
                break;
            }

            dart->pos.x = item->pos.x + x;
            dart->pos.z = item->pos.z + z;
            Item_Initialise(dart_item_num);
            Item_AddActive(dart_item_num);
            dart->status = IS_ACTIVE;

            int16_t fx_num = Effect_Create(dart->room_num);
            if (fx_num != NO_ITEM) {
                FX *fx = &g_Effects[fx_num];
                fx->pos = dart->pos;
                fx->rot = dart->rot;
                fx->speed = 0;
                fx->frame_num = 0;
                fx->counter = 0;
                fx->object_id = O_DART_EFFECT;
                Sound_Effect(SFX_DARTS, &fx->pos, SPM_NORMAL);
            }
        }
    }
    Item_Animate(item);
}
