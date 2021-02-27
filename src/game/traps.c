#include "game/control.h"
#include "game/game.h"
#include "game/items.h"
#include "game/traps.h"
#include "game/vars.h"
#include "util.h"

void LavaBurn(ITEM_INFO* item)
{
    if (item->hit_points < 0) {
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO* floor = GetFloor(item->pos.x, 32000, item->pos.z, &room_num);

    if (item->floor != GetHeight(floor, item->pos.x, 32000, item->pos.z)) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;
    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->object_number = O_FLAME;
            fx->frame_number =
                (Objects[O_FLAME].nmeshes * GetRandomControl()) / 0x7FFF;
            fx->counter = -1 - GetRandomControl() * 24 / 0x7FFF;
        }
    }
}

void T1MInjectGameTraps()
{
    INJECT(0x0043B430, LavaBurn);
}
