#include "game/traps/dart.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/items.h"
#include "game/vars.h"

void SetupDart(OBJECT_INFO *obj)
{
    obj->collision = ObjectCollision;
    obj->control = DartsControl;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = 1;
}

void SetupDartEffect(OBJECT_INFO *obj)
{
    obj->control = DartEffectControl;
    obj->draw_routine = DrawSpriteItem;
}

void DartsControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->touch_bits) {
        LaraItem->hit_points -= 50;
        LaraItem->hit_status = 1;
        DoBloodSplat(
            item->pos.x, item->pos.y, item->pos.z, LaraItem->speed,
            LaraItem->pos.y_rot, LaraItem->room_number);
    }
    AnimateItem(item);

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (item->room_number != room_num) {
        ItemNewRoom(item_num, room_num);
    }

    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y >= item->floor) {
        KillItem(item_num);
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->pos = item->pos;
            fx->speed = 0;
            fx->counter = 6;
            fx->frame_number = -3 * GetRandomControl() / 0x8000;
            fx->object_number = O_RICOCHET1;
        }
    }
}

void DartEffectControl(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter >= 3) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}
