#include "game/effects.h"
#include "game/room.h"
#include "global/const.h"

#include <libtrx/game/math.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

static void M_Control(const int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->rot.y += 9 * DEG_1;
    effect->rot.x += 13 * DEG_1;

    int32_t x = effect->pos.x + ((Math_Sin(effect->rot.y) * 11) >> W2V_SHIFT);
    int32_t y = effect->pos.y - effect->speed;
    int32_t z = effect->pos.z + ((Math_Cos(effect->rot.x) * 8) >> W2V_SHIFT);

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    if (!sector || !(Room_Get(room_num)->flags & RF_UNDERWATER)) {
        Effect_Kill(effect_num);
        return;
    }

    const int32_t height = Room_GetCeiling(sector, x, y, z);
    if (height == NO_HEIGHT || y <= height) {
        Effect_Kill(effect_num);
        return;
    }

    if (effect->room_num != room_num) {
        Effect_NewRoom(effect_num, room_num);
    }
    effect->pos.x = x;
    effect->pos.y = y;
    effect->pos.z = z;
}

REGISTER_OBJECT(O_BUBBLES_1, M_Setup)
