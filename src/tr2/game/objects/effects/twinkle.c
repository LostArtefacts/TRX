#include "game/objects/effects/twinkle.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/math.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define DISAPPEAR_RANGE STEP_L

static XYZ_32 M_GetTargetPos(const ITEM *item);
static void M_NudgeTowardsItem(FX *fx, const XYZ_32 *target_pos);
static bool M_ShouldDisappear(const FX *fx, const XYZ_32 *target_pos);

static XYZ_32 M_GetTargetPos(const ITEM *const item)
{
    XYZ_32 pos = item->pos;

    if (item->object_id == O_DRAGON_FRONT) {
        const int32_t c = Math_Cos(item->rot.y);
        const int32_t s = Math_Sin(item->rot.y);
        pos.x += (c * 1100 + s * 490) >> W2V_SHIFT;
        pos.z += (c * 490 - s * 1100) >> W2V_SHIFT;
        pos.y -= 540;
    }

    return pos;
}

static void M_NudgeTowardsItem(FX *const fx, const XYZ_32 *const target_pos)
{
    fx->pos.x += (target_pos->x - fx->pos.x) >> 4;
    fx->pos.y += (target_pos->y - fx->pos.y) >> 4;
    fx->pos.z += (target_pos->z - fx->pos.z) >> 4;
}

static bool M_ShouldDisappear(
    const FX *const fx, const XYZ_32 *const target_pos)
{
    const int32_t dx = ABS(fx->pos.x - target_pos->x);
    const int32_t dy = ABS(fx->pos.y - target_pos->y);
    const int32_t dz = ABS(fx->pos.z - target_pos->z);
    return dx < DISAPPEAR_RANGE && dy < DISAPPEAR_RANGE && dz < DISAPPEAR_RANGE;
}

void __cdecl Twinkle_Control(const int16_t fx_num)
{
    FX *const fx = &g_Effects[fx_num];
    fx->frame_num--;
    if (fx->frame_num <= g_Objects[fx->object_id].mesh_count) {
        fx->frame_num = 0;
    }

    if (fx->counter < 0) {
        fx->counter++;
        if (fx->counter == 0) {
            Effect_Kill(fx_num);
        }
        return;
    }

    const ITEM *const item = Item_Get(fx->counter);
    const XYZ_32 target_pos = M_GetTargetPos(item);
    M_NudgeTowardsItem(fx, &target_pos);
    if (M_ShouldDisappear(fx, &target_pos)) {
        Effect_Kill(fx_num);
    }
}
