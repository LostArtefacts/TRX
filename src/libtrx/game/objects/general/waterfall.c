#include "game/effects.h"
#include "game/lara/common.h"
#include "game/output.h"
#include "game/random.h"
#include "game/sound.h"
#include "version.h"

#define M_RANGE (WALL_L * 10) // = 10240

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    Output_CalculateLight(item->pos, item->room_num);
    if (g_TRVersion == 2 && Item_IsNearby(item, lara_item, M_RANGE)) {
        Sound_Effect(SFX_WATERFALL_LOOP, &item->pos, SPM_NORMAL);
    }

    const int16_t effect_num = Effect_Create(item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_SPLASH_1;
        effect->pos.x =
            item->pos.x + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
        effect->pos.y = item->pos.y;
        effect->pos.z =
            item->pos.z + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->shade = Output_GetLightAdder();
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_WATERFALL, M_Setup)
