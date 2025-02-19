#include "game/effects.h"
#include "game/items.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define DISAPPEAR_RANGE STEP_L

static XYZ_32 M_GetTargetPos(const ITEM *item);
static void M_NudgeTowardsItem(EFFECT *effect, const XYZ_32 *target_pos);
static bool M_ShouldDisappear(const EFFECT *effect, const XYZ_32 *target_pos);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

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

static void M_NudgeTowardsItem(
    EFFECT *const effect, const XYZ_32 *const target_pos)
{
    effect->pos.x += (target_pos->x - effect->pos.x) >> 4;
    effect->pos.y += (target_pos->y - effect->pos.y) >> 4;
    effect->pos.z += (target_pos->z - effect->pos.z) >> 4;
}

static bool M_ShouldDisappear(
    const EFFECT *const effect, const XYZ_32 *const target_pos)
{
    const int32_t dx = ABS(effect->pos.x - target_pos->x);
    const int32_t dy = ABS(effect->pos.y - target_pos->y);
    const int32_t dz = ABS(effect->pos.z - target_pos->z);
    return dx < DISAPPEAR_RANGE && dy < DISAPPEAR_RANGE && dz < DISAPPEAR_RANGE;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->frame_num--;
    if (effect->frame_num <= Object_Get(effect->object_id)->mesh_count) {
        effect->frame_num = 0;
    }

    if (effect->counter < 0) {
        effect->counter++;
        if (effect->counter == 0) {
            Effect_Kill(effect_num);
        }
        return;
    }

    const ITEM *const item = Item_Get(effect->counter);
    const XYZ_32 target_pos = M_GetTargetPos(item);
    M_NudgeTowardsItem(effect, &target_pos);
    if (M_ShouldDisappear(effect, &target_pos)) {
        Effect_Kill(effect_num);
    }
}

REGISTER_OBJECT(O_TWINKLE, M_Setup)
