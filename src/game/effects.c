#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/misc.h"
#include "game/sphere.h"
#include "game/vars.h"
#include "config.h"
#include "util.h"

int32_t ItemNearLara(PHD_3DPOS* pos, int32_t distance)
{
    int32_t x = pos->x - LaraItem->pos.x;
    int32_t y = pos->y - LaraItem->pos.y;
    int32_t z = pos->z - LaraItem->pos.z;

    if (x >= -distance && x <= distance && z >= -distance && z <= distance
        && y >= -WALL_L * 3 && y <= WALL_L * 3
        && SQUARE(x) + SQUARE(z) <= SQUARE(distance)) {
        int16_t* bounds = GetBoundsAccurate(LaraItem);
        if (y >= bounds[FRAME_BOUND_MIN_Y]
            && y <= bounds[FRAME_BOUND_MAX_Y] + 100) {
            return 1;
        }
    }

    return 0;
}

int16_t DoBloodSplat(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num)
{
    int16_t fx_num = CreateEffect(room_num);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = x;
        fx->pos.y = y;
        fx->pos.z = z;
        fx->pos.y_rot = direction;
        fx->object_number = O_BLOOD1;
        fx->frame_number = 0;
        fx->counter = 0;
        fx->speed = speed;
    }
    return fx_num;
}

void ControlBlood1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->pos.x += (phd_sin(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->pos.z += (phd_cos(fx->pos.y_rot) * fx->speed) >> W2V_SHIFT;
    fx->counter++;
    if (fx->counter == 4) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void ControlExplosion1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->counter++;
    if (fx->counter == 2) {
        fx->counter = 0;
        fx->frame_number--;
        if (fx->frame_number <= Objects[fx->object_number].nmeshes) {
            KillEffect(fx_num);
        }
    }
}

void Richochet(GAME_VECTOR* pos)
{
    int16_t fx_num = CreateEffect(pos->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO* fx = &Effects[fx_num];
        fx->pos.x = pos->x;
        fx->pos.y = pos->y;
        fx->pos.z = pos->z;
        fx->counter = 4;
        fx->object_number = O_RICOCHET1;
        fx->frame_number = -3 * GetRandomDraw() / 32768;
        SoundEffect(10, &fx->pos, 0);
    }
}

void FxLaraBubbles(ITEM_INFO* item)
{
#ifdef T1M_FEAT_CHEATS
    // NOTE: until we get Robolara, it makes sense for her to breathe underwater
    if (Lara.water_status == LWS_CHEAT
        && !(RoomInfo[LaraItem->room_number].flags & RF_UNDERWATER)) {
        return;
    }
#endif

    int32_t count = (GetRandomDraw() * 3) / 0x8000;
    if (!count) {
        return;
    }

    SoundEffect(37, &item->pos, SFX_UNDERWATER);

    PHD_VECTOR offset;
    offset.x = 0;
    offset.y = 0;
    offset.z = 50;
    GetJointAbsPosition(item, &offset, LM_HEAD);

    for (int i = 0; i < count; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->pos.x = offset.x;
            fx->pos.y = offset.y;
            fx->pos.z = offset.z;
            fx->object_number = O_BUBBLES1;
            fx->frame_number = -((GetRandomDraw() * 3) / 0x8000);
            fx->speed = 10 + ((GetRandomDraw() * 6) / 0x8000);
        }
    }
}

void ControlBubble1(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];
    fx->pos.y_rot += 9 * PHD_DEGREE;
    fx->pos.x_rot += 13 * PHD_DEGREE;

    int32_t x = fx->pos.x + ((phd_sin(fx->pos.y_rot) * 11) >> W2V_SHIFT);
    int32_t y = fx->pos.y - fx->speed;
    int32_t z = fx->pos.z + ((phd_cos(fx->pos.x_rot) * 8) >> W2V_SHIFT);

    int16_t room_num = fx->room_number;
    FLOOR_INFO* floor = GetFloor(x, y, z, &room_num);
    if (!floor || !(RoomInfo[room_num].flags & RF_UNDERWATER)) {
        KillEffect(fx_num);
        return;
    }

    int32_t height = GetCeiling(floor, x, y, z);
    if (height == NO_HEIGHT || y <= height) {
        KillEffect(fx_num);
        return;
    }

    if (fx->room_number != room_num) {
        EffectNewRoom(fx_num, room_num);
    }
    fx->pos.x = x;
    fx->pos.y = y;
    fx->pos.z = z;
}

void FxChainBlock(ITEM_INFO* item)
{
#ifdef T1M_FEAT_OG_FIXES
    if (T1MConfig.fix_tihocan_secret_sound) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
        return;
    }
#endif

    if (FlipTimer == 0) {
        SoundEffect(173, NULL, 0);
    }

    FlipTimer++;
    if (FlipTimer == 55) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
    }
}

void T1MInjectGameEffects()
{
    INJECT(0x0041A210, ItemNearLara);
    INJECT(0x0041A310, DoBloodSplat);
    INJECT(0x0041A370, ControlBlood1);
    INJECT(0x0041A400, ControlExplosion1);
    INJECT(0x0041A670, FxLaraBubbles);
    INJECT(0x0041A760, ControlBubble1);
    INJECT(0x0041AD00, FxChainBlock);
}
