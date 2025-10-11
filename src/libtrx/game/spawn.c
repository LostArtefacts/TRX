#include "game/spawn.h"

#include "game/effects.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"

void Spawn_Splash(const ITEM *const item)
{
    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    for (int32_t i = 0; i < 10; i++) {
        const int16_t effect_num = Effect_Create(room_num);
        if (effect_num == NO_EFFECT) {
            continue;
        }

        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_SPLASH_1;
        effect->pos.x = item->pos.x;
        effect->pos.y = water_height;
        effect->pos.z = item->pos.z;
        effect->rot.y = 2 * Random_GetDraw() + DEG_180;
        effect->speed = Random_GetDraw() / 256;
        effect->frame_num = 0;
    }
}

void Spawn_Ricochet(const GAME_VECTOR *const pos)
{
    const int16_t effect_num = Effect_Create(pos->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_RICOCHET;
        effect->pos = pos->pos;
        effect->counter = 4;
        effect->frame_num = -3 * Random_GetDraw() / 0x8000;
        Sound_Effect(SFX_LARA_RICOCHET, &effect->pos, SPM_NORMAL);
    }
}
