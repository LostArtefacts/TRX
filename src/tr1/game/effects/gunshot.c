#include "game/effects/gunshot.h"

#include "game/collide.h"
#include "game/effects.h"
#include "game/effects/blood.h"
#include "game/objects/effects/ricochet.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

int16_t Effect_GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    int16_t fx_num = Effect_Create(room_num);
    if (fx_num != NO_ITEM) {
        FX *fx = &g_Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->room_num = room_num;
        fx->rot.x = 0;
        fx->rot.y = y_rot;
        fx->rot.z = 0;
        fx->counter = 3;
        fx->frame_num = 0;
        fx->object_id = O_GUN_FLASH;
        fx->shade = 4096;
    }
    return fx_num;
}

int16_t Effect_GunShotHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    XYZ_32 pos = {
        .x = 0,
        .y = 0,
        .z = 0,
    };
    Collide_GetJointAbsPosition(
        g_LaraItem, &pos, (Random_GetControl() * 25) / 0x7FFF);
    Effect_Blood(
        pos.x, pos.y, pos.z, g_LaraItem->speed, g_LaraItem->rot.y,
        g_LaraItem->room_num);
    Sound_Effect(SFX_LARA_BULLETHIT, &g_LaraItem->pos, SPM_NORMAL);
    return Effect_GunShot(x, y, z, speed, y_rot, room_num);
}

int16_t Effect_GunShotMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num)
{
    GAME_VECTOR pos;
    pos.x = g_LaraItem->pos.x
        + ((Random_GetDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.y = g_LaraItem->floor;
    pos.z = g_LaraItem->pos.z
        + ((Random_GetDraw() - 0x4000) * (WALL_L / 2)) / 0x7FFF;
    pos.room_num = g_LaraItem->room_num;
    Ricochet_Spawn(&pos);
    return Effect_GunShot(x, y, z, speed, y_rot, room_num);
}
