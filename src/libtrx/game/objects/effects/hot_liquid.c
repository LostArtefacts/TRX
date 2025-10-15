#include "game/const.h"
#include "game/effects.h"
#include "game/objects.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"

static void M_Control(const int16_t effect_num)
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
        effect->object_id = O_SPLASH_1;
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

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->semi_transparent = true;
}

REGISTER_OBJECT(O_HOT_LIQUID, M_Setup)
