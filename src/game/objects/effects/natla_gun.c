#include "game/objects/effects/natla_gun.h"

#include "game/effects.h"
#include "game/room.h"
#include "global/vars.h"
#include "math/math.h"

void NatlaGun_Setup(OBJECT *obj)
{
    obj->control = NatlaGun_Control;
}

void NatlaGun_Control(int16_t fx_num)
{
    FX *fx = &g_Effects[fx_num];
    OBJECT *object = &g_Objects[fx->object_id];

    fx->frame_num--;
    if (fx->frame_num <= object->nmeshes) {
        Effect_Kill(fx_num);
    }

    if (fx->frame_num == -1) {
        return;
    }

    int32_t z = fx->pos.z + ((fx->speed * Math_Cos(fx->rot.y)) >> W2V_SHIFT);
    int32_t x = fx->pos.x + ((fx->speed * Math_Sin(fx->rot.y)) >> W2V_SHIFT);
    int32_t y = fx->pos.y;
    int16_t room_num = fx->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);

    if (y >= Room_GetHeight(sector, x, y, z)
        || y <= Room_GetCeiling(sector, x, y, z)) {
        return;
    }

    fx_num = Effect_Create(room_num);
    if (fx_num != NO_ITEM) {
        FX *newfx = &g_Effects[fx_num];
        newfx->pos.x = x;
        newfx->pos.y = y;
        newfx->pos.z = z;
        newfx->rot.y = fx->rot.y;
        newfx->room_num = room_num;
        newfx->speed = fx->speed;
        newfx->frame_num = 0;
        newfx->object_id = O_MISSILE_1;
    }
}
