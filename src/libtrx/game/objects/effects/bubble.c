#include "game/effects.h"
#include "game/objects.h"
#include "game/output.h"

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    effect->rot.y += 9 * DEG_1;
    effect->rot.x += 13 * DEG_1;

    const XYZ_32 pos = {
        .x = effect->pos.x + ((Math_Sin(effect->rot.y) * 11) >> W2V_SHIFT),
        .y = effect->pos.y - effect->speed,
        .z = effect->pos.z + ((Math_Cos(effect->rot.x) * 8) >> W2V_SHIFT),
    };

    int16_t room_num = effect->room_num;
    const SECTOR *const sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    if (sector == nullptr || (Room_Get(room_num)->flags & RF_UNDERWATER) == 0) {
        Effect_Kill(effect_num);
        return;
    }

    const int32_t ceiling = Room_GetCeiling(sector, pos.x, pos.y, pos.z);
    if (ceiling == NO_HEIGHT || pos.y <= ceiling) {
        Effect_Kill(effect_num);
        return;
    }

    if (effect->room_num != room_num) {
        Effect_NewRoom(effect_num, room_num);
    }
    effect->pos = pos;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    if (obj->loaded) {
        for (int32_t i = 0; i < -obj->mesh_count; i++) {
            Output_GetSpriteTexture(obj->mesh_idx + i)->flags = VERT_ABS_SPRITE;
        }
    }
}

REGISTER_OBJECT(O_BUBBLE_1, M_Setup)
