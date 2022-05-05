#include "game/objects/effects/bubble.h"

#include "3dsystem/phd_math.h"
#include "game/effects.h"
#include "game/room.h"
#include "global/vars.h"

void Bubble_Setup(OBJECT_INFO *obj)
{
    obj->control = Bubble_Control;
}

void Bubble_Control(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->pos.y_rot += 9 * PHD_DEGREE;
    fx->pos.x_rot += 13 * PHD_DEGREE;

    int32_t x = fx->pos.x + ((phd_sin(fx->pos.y_rot) * 11) >> W2V_SHIFT);
    int32_t y = fx->pos.y - fx->speed;
    int32_t z = fx->pos.z + ((phd_cos(fx->pos.x_rot) * 8) >> W2V_SHIFT);

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    if (!floor || !(g_RoomInfo[room_num].flags & RF_UNDERWATER)) {
        Effect_Kill(fx_num);
        return;
    }

    int32_t height = Room_GetCeiling(floor, x, y, z);
    if (height == NO_HEIGHT || y <= height) {
        Effect_Kill(fx_num);
        return;
    }

    if (fx->room_number != room_num) {
        Effect_NewRoom(fx_num, room_num);
    }
    fx->pos.x = x;
    fx->pos.y = y;
    fx->pos.z = z;
}
