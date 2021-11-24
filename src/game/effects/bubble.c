#include "game/effects/bubble.h"

#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/items.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/sphere.h"
#include "global/vars.h"

void SetupBubble(OBJECT_INFO *obj)
{
    obj->control = ControlBubble1;
}

void LaraBubbles(ITEM_INFO *item)
{
    // XXX: until we get Robolara, it makes sense for her to breathe underwater
    if (g_Lara.water_status == LWS_CHEAT
        && !(g_RoomInfo[g_LaraItem->room_number].flags & RF_UNDERWATER)) {
        return;
    }

    int32_t count = (Random_GetDraw() * 3) / 0x8000;
    if (!count) {
        return;
    }

    Sound_Effect(SFX_LARA_BUBBLES, &item->pos, SPM_UNDERWATER);

    PHD_VECTOR offset;
    offset.x = 0;
    offset.y = 0;
    offset.z = 50;
    GetJointAbsPosition(item, &offset, LM_HEAD);

    for (int i = 0; i < count; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = offset.x;
            fx->pos.y = offset.y;
            fx->pos.z = offset.z;
            fx->object_number = O_BUBBLES1;
            fx->frame_number = -((Random_GetDraw() * 3) / 0x8000);
            fx->speed = 10 + ((Random_GetDraw() * 6) / 0x8000);
        }
    }
}

void ControlBubble1(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    fx->pos.y_rot += 9 * PHD_DEGREE;
    fx->pos.x_rot += 13 * PHD_DEGREE;

    int32_t x = fx->pos.x + ((phd_sin(fx->pos.y_rot) * 11) >> W2V_SHIFT);
    int32_t y = fx->pos.y - fx->speed;
    int32_t z = fx->pos.z + ((phd_cos(fx->pos.x_rot) * 8) >> W2V_SHIFT);

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    if (!floor || !(g_RoomInfo[room_num].flags & RF_UNDERWATER)) {
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
