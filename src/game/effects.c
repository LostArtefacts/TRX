#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/vars.h"
#include "config.h"
#include "util.h"

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
    INJECT(0x0041A760, ControlBubble1);
    INJECT(0x0041AD00, FxChainBlock);
}
