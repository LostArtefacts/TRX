#include "game/objects/effects/hot_liquid.h"

#include "game/effects.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

void HotLiquid_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    OBJECT *const obj = Object_Get(O_HOT_LIQUID);

    effect->frame_num--;
    if (effect->frame_num <= obj->mesh_count) {
        effect->frame_num = 0;
    }

    effect->pos.y += effect->fall_speed;
    effect->fall_speed += GRAVITY;

    int16_t room_num = effect->room_num;
    const SECTOR *const sector =
        Room_GetSector(effect->pos.x, effect->pos.y, effect->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, effect->pos.x, effect->pos.y, effect->pos.z);

    if (effect->pos.y >= height) {
        Sound_Effect(SFX_WATERFALL_2, &effect->pos, SPM_NORMAL);
        effect->object_id = O_SPLASH;
        effect->pos.y = height;
        effect->rot.y = 2 * Random_GetDraw();
        effect->fall_speed = 0;
        effect->speed = 50;
        return;
    }

    if (effect->room_num != room_num) {
        Effect_NewRoom(effect_num, room_num);
    }
    Sound_Effect(SFX_BOWL_POUR, &effect->pos, SPM_NORMAL);
}

void HotLiquid_Setup(void)
{
    OBJECT *const obj = Object_Get(O_HOT_LIQUID);
    obj->control_func = HotLiquid_Control;
    obj->semi_transparent = 1;
}
