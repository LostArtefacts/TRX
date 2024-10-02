#include "game/objects/effects/bubble.h"

#include "game/effects.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

void Bubble_Setup(OBJECT *obj)
{
    obj->control = Bubble_Control;
}

void Bubble_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    fx->rot.y += 9 * PHD_DEGREE;
    fx->rot.x += 13 * PHD_DEGREE;

    int32_t x = fx->pos.x + ((Math_Sin(fx->rot.y) * 11) >> W2V_SHIFT);
    int32_t y = fx->pos.y - fx->speed;
    int32_t z = fx->pos.z + ((Math_Cos(fx->rot.x) * 8) >> W2V_SHIFT);

    int16_t room_num = fx->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    if (!sector || !(g_RoomInfo[room_num].flags & RF_UNDERWATER)) {
        Effect_Kill(fx_num);
        return;
    }

    const int32_t height = Room_GetCeiling(sector, x, y, z);
    if (height == NO_HEIGHT || y <= height) {
        Effect_Kill(fx_num);
        return;
    }

    if (fx->room_num != room_num) {
        Effect_NewRoom(fx_num, room_num);
    }
    fx->pos.x = x;
    fx->pos.y = y;
    fx->pos.z = z;
}
