#include "game/control.h"
#include "game/game.h"
#include "game/items.h"
#include "game/people.h"
#include "game/vars.h"
#include "util.h"

#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224

int32_t Targetable(ITEM_INFO* item, AI_INFO* info)
{
    if (!info->ahead || info->distance >= PEOPLE_SHOOT_RANGE) {
        return 0;
    }

    GAME_VECTOR start;
    start.x = item->pos.x;
    start.y = item->pos.y - STEP_L * 3;
    start.z = item->pos.z;
    start.room_number = item->room_number;

    GAME_VECTOR target;
    target.x = LaraItem->pos.x;
    target.y = LaraItem->pos.y - STEP_L * 3;
    target.z = LaraItem->pos.z;

    return LOS(&start, &target);
}

void ControlGunShot(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter--;
    if (!fx->counter) {
        KillEffect(fx_num);
        return;
    }
    fx->pos.z_rot = GetRandomControl();
}

void T1MInjectGamePeople()
{
    INJECT(0x00430D80, Targetable);
    INJECT(0x00430E00, ControlGunShot);
}
