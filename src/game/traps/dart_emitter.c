#include "game/traps/dart_emitter.h"

#include "game/control.h"
#include "game/items.h"
#include "game/sound.h"
#include "game/vars.h"

void SetupDartEmitter(OBJECT_INFO *obj)
{
    obj->control = DartEmitterControl;
}

void DartEmitterControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if (TriggerActive(item)) {
        if (item->current_anim_state == DART_EMITTER_IDLE) {
            item->goal_anim_state = DART_EMITTER_FIRE;
        }
    } else {
        if (item->current_anim_state == DART_EMITTER_FIRE) {
            item->goal_anim_state = DART_EMITTER_IDLE;
        }
    }

    if (item->current_anim_state == DART_EMITTER_FIRE
        && item->frame_number == Anims[item->anim_number].frame_base) {
        int16_t dart_item_num = CreateItem();
        if (dart_item_num != NO_ITEM) {
            ITEM_INFO *dart = &Items[dart_item_num];
            dart->object_number = O_DARTS;
            dart->room_number = item->room_number;
            dart->shade = -1;
            dart->pos.y_rot = item->pos.y_rot;
            dart->pos.y = item->pos.y - WALL_L / 2;

            int32_t x = 0;
            int32_t z = 0;
            switch (dart->pos.y_rot) {
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
            InitialiseItem(dart_item_num);
            AddActiveItem(dart_item_num);
            dart->status = IS_ACTIVE;

            int16_t fx_num = CreateEffect(dart->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &Effects[fx_num];
                fx->pos = dart->pos;
                fx->speed = 0;
                fx->frame_number = 0;
                fx->counter = 0;
                fx->object_number = O_DART_EFFECT;
                SoundEffect(SFX_DARTS, &fx->pos, SPM_NORMAL);
            }
        }
    }
    AnimateItem(item);
}
