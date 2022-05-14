#include "game/objects/traps/dart.h"

#include "game/effects.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

typedef enum {
    DART_EMITTER_IDLE = 0,
    DART_EMITTER_FIRE = 1,
} DART_EMITTER_STATE;

void Dart_Setup(OBJECT_INFO *obj)
{
    obj->collision = Object_Collision;
    obj->control = Dart_Control;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = 1;
}

void Dart_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->touch_bits) {
        g_LaraItem->hit_points -= 50;
        g_LaraItem->hit_status = 1;
        Effect_Blood(
            item->pos.x, item->pos.y, item->pos.z, g_LaraItem->speed,
            g_LaraItem->pos.y_rot, g_LaraItem->room_number);
    }

    int32_t old_x = item->pos.x;
    int32_t old_z = item->pos.z;
    Item_Animate(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_number != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    item->floor = Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y >= item->floor) {
        Item_Kill(item_num);
        int16_t fx_num = Effect_Create(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = old_x;
            fx->pos.y = item->pos.y;
            fx->pos.z = old_z;
            fx->speed = 0;
            fx->counter = 6;
            fx->frame_number = -3 * Random_GetControl() / 0x8000;
            fx->object_number = O_RICOCHET1;
        }
    }
}

void DartEffect_Setup(OBJECT_INFO *obj)
{
    obj->control = DartEffect_Control;
    obj->draw_routine = Object_DrawSpriteItem;
}

void DartEffect_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->counter++;
    if (fx->counter >= 3) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= g_Objects[fx->object_number].nmeshes) {
            Effect_Kill(fx_num);
        }
    }
}

void DartEmitter_Setup(OBJECT_INFO *obj)
{
    obj->control = DartEmitter_Control;
}

void DartEmitter_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

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
        && item->frame_number == g_Anims[item->anim_number].frame_base) {
        int16_t dart_item_num = Item_Create();
        if (dart_item_num != NO_ITEM) {
            ITEM_INFO *dart = &g_Items[dart_item_num];
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
            Item_Initialise(dart_item_num);
            Item_AddActive(dart_item_num);
            dart->status = IS_ACTIVE;

            int16_t fx_num = Effect_Create(dart->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &g_Effects[fx_num];
                fx->pos = dart->pos;
                fx->speed = 0;
                fx->frame_number = 0;
                fx->counter = 0;
                fx->object_number = O_DART_EFFECT;
                Sound_Effect(SFX_DARTS, &fx->pos, SPM_NORMAL);
            }
        }
    }
    Item_Animate(item);
}
